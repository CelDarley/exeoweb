#include <esp_now.h>
#include <WiFi.h>

// Variável para armazenar o endereço MAC do transmissor
uint8_t transmitterAddress[6];
bool receptorAcordado = true; // Estado inicial do receptor

// Função para enviar resposta
void enviarResposta(const char* mensagem) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, transmitterAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Erro ao adicionar peer");
    return;
  }

  esp_err_t result = esp_now_send(transmitterAddress, (uint8_t *)mensagem, strlen(mensagem));
  
  if (result == ESP_OK) {
    Serial.println("Resposta enviada com sucesso");
  } else {
    Serial.println("Erro ao enviar resposta");
  }

  esp_now_del_peer(transmitterAddress);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Status do último pacote enviado: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso" : "Falha");
}

void OnDataRecv(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, int data_len) {
  memcpy(transmitterAddress, esp_now_info->src_addr, 6);

  Serial.print("Comando recebido: ");
  for(int i = 0; i < data_len; i++) {
    Serial.print((char)data[i]);
  }
  Serial.println();

  if (data[0] == '1') { // Acordar
    if (receptorAcordado) {
      Serial.println("Já estou acordado");
      enviarResposta("Já estou acordado!");
    } else {
      Serial.println("Acordando...");
      receptorAcordado = true;
      enviarResposta("Acordei");
    }
  } 
  else if (data[0] == '2') { // Dormir
    if (!receptorAcordado) {
      Serial.println("Já estou dormindo");
      enviarResposta("Já estou dormindo!");
    } else {
      Serial.println("Preparando para dormir");
      receptorAcordado = false;
      enviarResposta("Vou dormir");
      delay(500);
      esp_deep_sleep_start();
    }
  }
  else if (data[0] == '0') { // Verificar status
    String status = receptorAcordado ? "Estou acordado" : "Estou dormindo";
    Serial.println("Enviando status: " + status);
    enviarResposta(status.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(OnDataRecv));
  esp_now_register_send_cb(OnDataSent);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);

  Serial.println("Receptor iniciado");
}

void loop() {
  // Loop vazio
}
