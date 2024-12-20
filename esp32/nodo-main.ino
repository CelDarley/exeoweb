#include <esp_now.h>
#include <WiFi.h>

// Endereço MAC do receptor (substitua pelos valores corretos)
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

volatile bool mensagemEntregue = false;
volatile bool aguardandoResposta = false;
String respostaReceptor = "";

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  mensagemEntregue = (status == ESP_NOW_SEND_SUCCESS);
  Serial.print("Status do último pacote enviado: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso" : "Falha");
}

void OnDataRecv(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, int len) {
  respostaReceptor = "";
  for (int i = 0; i < len; i++) {
    respostaReceptor += (char)data[i];
  }
  Serial.print("Resposta do receptor: ");
  Serial.println(respostaReceptor);
  aguardandoResposta = false;
}

bool enviarComando(const char* comando, int maxTentativas = 5) {
  int tentativas = 0;
  bool sucesso = false;
  
  while (!sucesso && tentativas < maxTentativas) {
    tentativas++;
    mensagemEntregue = false;
    aguardandoResposta = true;
    respostaReceptor = "";

    if (tentativas > 1) {
      Serial.print("Tentativa ");
      Serial.print(tentativas);
      Serial.println(" de envio...");
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Falha ao adicionar peer");
      continue;
    }

    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)comando, strlen(comando));
    
    if (result == ESP_OK) {
      int timeout = 0;
      while (aguardandoResposta && timeout < 20) {
        delay(100);
        timeout++;
      }

      if (mensagemEntregue && !respostaReceptor.isEmpty()) {
        sucesso = true;
      }
    }

    esp_now_del_peer(receiverAddress);

    if (!sucesso) {
      delay(1000);
    }
  }

  if (!sucesso) {
    Serial.println("Falha após todas as tentativas de envio");
  }

  return sucesso;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(OnDataRecv));

  Serial.println("Transmissor iniciado");
  Serial.println("Comandos disponíveis:");
  Serial.println("0 - Verificar status do receptor");
  Serial.println("1 - Acordar receptor");
  Serial.println("2 - Fazer receptor dormir");
}

void loop() {
  if (Serial.available() > 0) {
    char comando = Serial.read();
    
    if (comando == '0') {
      Serial.println("Verificando status do receptor...");
      enviarComando("0");
    }
    else if (comando == '1') {
      Serial.println("Enviando comando para acordar o receptor...");
      enviarComando("1");
    }
    else if (comando == '2') {
      Serial.println("Enviando comando para fazer o receptor dormir...");
      enviarComando("2");
    }
    
    while(Serial.available()) {
      Serial.read();
    }
  }
}
