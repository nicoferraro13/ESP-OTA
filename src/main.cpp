#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "AppConfig.h"

enum class PortalMode : uint8_t {
  AlwaysOn = 0,
  Auto = 1,
};

struct RuntimeConfig {
  String wifiSsid;
  String wifiPassword;
  String manifestUrl;
  PortalMode portalMode = PortalMode::AlwaysOn;
};

struct OtaState {
  bool updateAvailable = false;
  bool startupCheckPending = true;
  String remoteVersion;
  String pendingBinUrl;
  String releaseNotes;
  String lastCheckMessage = "Todavia no se reviso el OTA.";
  unsigned long lastCheckMillis = 0;
};

WebServer server(80);
Preferences preferences;
RuntimeConfig config;
OtaState otaState;

bool apEnabled = false;
bool apManualOverride = false;
bool staConnecting = false;
bool staPaused = false;
bool previousStaConnected = false;
bool ledState = false;

unsigned long lastBlinkMillis = 0;
unsigned long lastWifiAttemptMillis = 0;

String uiMessage;

String htmlEscape(const String& input) {
  String output;
  output.reserve(input.length() + 16);

  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    switch (c) {
      case '&':
        output += F("&amp;");
        break;
      case '<':
        output += F("&lt;");
        break;
      case '>':
        output += F("&gt;");
        break;
      case '"':
        output += F("&quot;");
        break;
      case '\'':
        output += F("&#39;");
        break;
      default:
        output += c;
        break;
    }
  }

  return output;
}

String formatBool(bool value, const char* whenTrue, const char* whenFalse) {
  return value ? String(whenTrue) : String(whenFalse);
}

String portalModeLabel(PortalMode mode) {
  return mode == PortalMode::AlwaysOn ? String("Siempre encendido") : String("Auto");
}

bool isStaConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool hasWifiCredentials() {
  return !config.wifiSsid.isEmpty();
}

void applyLedOutput(bool on) {
  const bool physicalLevel = AppConfig::LED_ACTIVE_HIGH ? on : !on;
  digitalWrite(AppConfig::LED_PIN, physicalLevel ? HIGH : LOW);
}

void saveConfig() {
  preferences.begin(AppConfig::PREF_NAMESPACE, false);
  preferences.putString(AppConfig::PREF_WIFI_SSID, config.wifiSsid);
  preferences.putString(AppConfig::PREF_WIFI_PASS, config.wifiPassword);
  preferences.putString(AppConfig::PREF_MANIFEST_URL, config.manifestUrl);
  preferences.putUChar(AppConfig::PREF_PORTAL_MODE, static_cast<uint8_t>(config.portalMode));
  preferences.end();
}

void loadConfig() {
  preferences.begin(AppConfig::PREF_NAMESPACE, true);
  config.wifiSsid = preferences.getString(AppConfig::PREF_WIFI_SSID, "");
  config.wifiPassword = preferences.getString(AppConfig::PREF_WIFI_PASS, "");
  config.manifestUrl = preferences.getString(AppConfig::PREF_MANIFEST_URL, "");

  const uint8_t storedMode =
      preferences.getUChar(AppConfig::PREF_PORTAL_MODE, static_cast<uint8_t>(PortalMode::AlwaysOn));
  config.portalMode = storedMode == static_cast<uint8_t>(PortalMode::Auto) ? PortalMode::Auto
                                                                            : PortalMode::AlwaysOn;
  preferences.end();
}

void setUiMessage(const String& message) {
  uiMessage = message;
  Serial.println(message);
}

void redirectHome() {
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

bool shouldApBeEnabled() {
  if (apManualOverride) {
    return true;
  }

  if (config.portalMode == PortalMode::AlwaysOn) {
    return true;
  }

  return !isStaConnected();
}

void startAccessPoint() {
  const wifi_mode_t desiredMode = hasWifiCredentials() ? WIFI_AP_STA : WIFI_AP;
  WiFi.mode(desiredMode);
  WiFi.softAPConfig(AppConfig::AP_IP, AppConfig::AP_GATEWAY, AppConfig::AP_SUBNET);
  WiFi.softAP(AppConfig::AP_SSID, AppConfig::AP_PASSWORD);
  apEnabled = true;
}

void stopAccessPoint() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(hasWifiCredentials() ? WIFI_STA : WIFI_OFF);
  apEnabled = false;
}

void refreshApState() {
  const bool desired = shouldApBeEnabled();
  if (desired && !apEnabled) {
    startAccessPoint();
  } else if (!desired && apEnabled) {
    stopAccessPoint();
  }
}

void beginStationConnection() {
  if (!hasWifiCredentials() || staPaused) {
    return;
  }

  WiFi.mode(apEnabled ? WIFI_AP_STA : WIFI_STA);
  WiFi.setHostname(AppConfig::DEVICE_NAME);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  staConnecting = true;
  lastWifiAttemptMillis = millis();

  Serial.print(F("Intentando conectar a Wi-Fi: "));
  Serial.println(config.wifiSsid);
}

void disconnectStation(bool eraseWifiFromRadio) {
  WiFi.disconnect(false, eraseWifiFromRadio);
  staConnecting = false;
  previousStaConnected = false;
}

String staStatusText() {
  if (isStaConnected()) {
    return String("Conectado a ") + WiFi.SSID();
  }

  if (staPaused) {
    return "Desconectado por el usuario";
  }

  if (staConnecting) {
    return "Intentando conectar";
  }

  if (hasWifiCredentials()) {
    return "Desconectado";
  }

  return "Sin Wi-Fi configurado";
}

bool parseManifest(const String& content, String& error) {
  String remoteVersion;
  String binUrl;
  String notes;

  int start = 0;
  while (start < content.length()) {
    int end = content.indexOf('\n', start);
    if (end < 0) {
      end = content.length();
    }

    String line = content.substring(start, end);
    line.trim();
    start = end + 1;

    if (line.isEmpty() || line.startsWith("#")) {
      continue;
    }

    const int separator = line.indexOf('=');
    if (separator <= 0) {
      continue;
    }

    String key = line.substring(0, separator);
    String value = line.substring(separator + 1);
    key.trim();
    value.trim();

    if (key == "version") {
      remoteVersion = value;
    } else if (key == "bin_url") {
      binUrl = value;
    } else if (key == "notes") {
      notes = value;
    }
  }

  if (remoteVersion.isEmpty()) {
    error = "El manifest no trae la clave version.";
    return false;
  }

  if (binUrl.isEmpty()) {
    error = "El manifest no trae la clave bin_url.";
    return false;
  }

  otaState.remoteVersion = remoteVersion;
  otaState.pendingBinUrl = binUrl;
  otaState.releaseNotes = notes;
  return true;
}

bool downloadTextFile(const String& url, String& content, String& error) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    error = "No se pudo abrir la URL.";
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    error = String("La descarga devolvio HTTP ") + code + ".";
    http.end();
    return false;
  }

  content = http.getString();
  http.end();
  return true;
}

bool checkForUpdate(bool manualTrigger) {
  otaState.lastCheckMillis = millis();
  otaState.startupCheckPending = false;

  if (!isStaConnected()) {
    otaState.lastCheckMessage = "No se pudo revisar OTA porque el ESP32 no esta conectado a Internet.";
    otaState.updateAvailable = false;
    otaState.remoteVersion = "";
    otaState.pendingBinUrl = "";
    otaState.releaseNotes = "";
    return false;
  }

  if (config.manifestUrl.isEmpty()) {
    otaState.lastCheckMessage = "Todavia no cargaste la URL del manifest OTA.";
    otaState.updateAvailable = false;
    otaState.remoteVersion = "";
    otaState.pendingBinUrl = "";
    otaState.releaseNotes = "";
    return false;
  }

  String manifestContent;
  String error;
  if (!downloadTextFile(config.manifestUrl, manifestContent, error)) {
    otaState.lastCheckMessage = String("Fallo la revision OTA: ") + error;
    otaState.updateAvailable = false;
    otaState.remoteVersion = "";
    otaState.pendingBinUrl = "";
    otaState.releaseNotes = "";
    return false;
  }

  if (!parseManifest(manifestContent, error)) {
    otaState.lastCheckMessage = String("El manifest OTA es invalido: ") + error;
    otaState.updateAvailable = false;
    otaState.remoteVersion = "";
    otaState.pendingBinUrl = "";
    otaState.releaseNotes = "";
    return false;
  }

  if (otaState.remoteVersion != AppConfig::FW_VERSION) {
    otaState.updateAvailable = true;
    otaState.lastCheckMessage =
        String("Hay una nueva version disponible: ") + otaState.remoteVersion +
        (manualTrigger ? "." : " (detectada automaticamente).");
    return true;
  }

  otaState.updateAvailable = false;
  otaState.pendingBinUrl = "";
  otaState.releaseNotes = "";
  otaState.lastCheckMessage = String("No hay una version nueva. Seguimos en ") + AppConfig::FW_VERSION + ".";
  return false;
}

bool performOta(String& error) {
  if (!isStaConnected()) {
    error = "No hay conexion Wi-Fi para descargar el firmware.";
    return false;
  }

  if (otaState.pendingBinUrl.isEmpty()) {
    error = "No hay un firmware pendiente para instalar.";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  if (!http.begin(client, otaState.pendingBinUrl)) {
    error = "No se pudo abrir la URL del firmware.";
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    error = String("La descarga del firmware devolvio HTTP ") + code + ".";
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  const size_t updateSize =
      contentLength > 0 ? static_cast<size_t>(contentLength) : static_cast<size_t>(UPDATE_SIZE_UNKNOWN);

  if (!Update.begin(updateSize)) {
    error = String("No se pudo iniciar Update. Codigo: ") + Update.getError();
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  const size_t written = Update.writeStream(*stream);

  if (contentLength > 0 && written != static_cast<size_t>(contentLength)) {
    error = String("Se escribieron ") + written + " bytes de " + contentLength + ".";
    http.end();
    return false;
  }

  if (!Update.end()) {
    error = String("Fallo el cierre del Update. Codigo: ") + Update.getError();
    http.end();
    return false;
  }

  if (!Update.isFinished()) {
    error = "El update no termino correctamente.";
    http.end();
    return false;
  }

  http.end();
  return true;
}

String buildPage() {
  const String staIp = isStaConnected() ? WiFi.localIP().toString() : String("-");
  const String apIp = apEnabled ? WiFi.softAPIP().toString() : String("-");
  const String manifestValue = htmlEscape(config.manifestUrl);
  const String wifiSsidValue = htmlEscape(config.wifiSsid);
  const String notesValue = otaState.releaseNotes.isEmpty() ? String("Sin notas.")
                                                            : htmlEscape(otaState.releaseNotes);
  const String manifestExample =
      String("version=") + AppConfig::FW_VERSION +
      "\nbin_url=https://raw.githubusercontent.com/nicoferraro13/ESP-OTA/main/ota/firmware.bin"
      "\nnotes=Tu descripcion";

  String html;
  html.reserve(8192);

  html += F(
      "<!doctype html><html lang='es'><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>ESP32 OTA</title><style>"
      "body{font-family:Arial,sans-serif;background:#f4f7fb;color:#18212c;margin:0;padding:16px;}"
      ".wrap{max-width:900px;margin:0 auto;}"
      ".card{background:#fff;border:1px solid #d7deea;border-radius:14px;padding:18px;margin-bottom:16px;"
      "box-shadow:0 8px 24px rgba(0,0,0,.05);}"
      "h1,h2{margin:0 0 12px 0;}h1{font-size:28px;}h2{font-size:20px;}"
      "p{margin:8px 0;line-height:1.45;}label{display:block;font-weight:600;margin:12px 0 6px;}"
      "input[type=text],input[type=password],select{width:100%;padding:12px;border:1px solid #bfcadd;"
      "border-radius:10px;box-sizing:border-box;font-size:14px;}"
      "button{margin-top:12px;background:#1456a0;color:#fff;border:0;border-radius:10px;padding:12px 16px;"
      "cursor:pointer;font-size:14px;}button.secondary{background:#5f6f85;}button.warn{background:#a6452a;}"
      ".pill{display:inline-block;padding:4px 10px;border-radius:999px;background:#edf3fb;margin-right:8px;"
      "margin-bottom:8px;}code{background:#eef2f7;padding:2px 5px;border-radius:4px;}"
      ".msg{background:#eef7e8;border:1px solid #cde4be;padding:12px;border-radius:10px;}"
      ".warnbox{background:#fff5ec;border:1px solid #f0d0b0;padding:12px;border-radius:10px;}"
      ".actions form{display:inline-block;margin-right:8px;margin-bottom:8px;}"
      "</style></head><body><div class='wrap'>");

  html += F("<div class='card'><h1>ESP32 OTA</h1>");
  html += F("<p>Firmware actual: <code>");
  html += htmlEscape(AppConfig::FW_VERSION);
  html += F("</code></p>");

  if (!uiMessage.isEmpty()) {
    html += F("<div class='msg'><strong>Ultimo mensaje:</strong> ");
    html += htmlEscape(uiMessage);
    html += F("</div>");
  }

  html += F("</div>");

  html += F("<div class='card'><h2>Estado actual</h2>");
  html += F("<p><span class='pill'>Portal AP: ");
  html += htmlEscape(portalModeLabel(config.portalMode));
  html += F("</span><span class='pill'>AP activo: ");
  html += htmlEscape(formatBool(apEnabled, "Si", "No"));
  html += F("</span><span class='pill'>Wi-Fi: ");
  html += htmlEscape(staStatusText());
  html += F("</span></p>");

  html += F("<p>SSID del AP: <code>");
  html += htmlEscape(AppConfig::AP_SSID);
  html += F("</code></p>");
  html += F("<p>IP del AP: <code>");
  html += htmlEscape(apIp);
  html += F("</code></p>");
  html += F("<p>IP en la red Wi-Fi: <code>");
  html += htmlEscape(staIp);
  html += F("</code></p>");

  if (apEnabled) {
    html += F("<p>Portal por AP: <code>http://");
    html += htmlEscape(apIp);
    html += F("</code></p>");
  }

  if (isStaConnected()) {
    html += F("<p>Portal por Wi-Fi: <code>http://");
    html += htmlEscape(staIp);
    html += F("</code></p>");
  }

  html += F("<p>Ultima revision OTA: ");
  html += htmlEscape(otaState.lastCheckMessage);
  html += F("</p></div>");

  html += F("<div class='card'><h2>Configuracion</h2>");
  html += F("<form method='post' action='/config/save'>");
  html += F("<label for='wifi_ssid'>SSID del Wi-Fi</label>");
  html += F("<input id='wifi_ssid' name='wifi_ssid' type='text' value='");
  html += wifiSsidValue;
  html += F("' placeholder='Tu red Wi-Fi'>");

  html += F("<label for='wifi_password'>Password del Wi-Fi</label>");
  html += F("<input id='wifi_password' name='wifi_password' type='password' value='' "
            "placeholder='Dejalo vacio para conservar la actual'>");

  html += F("<label for='manifest_url'>URL del manifest OTA</label>");
  html += F("<input id='manifest_url' name='manifest_url' type='text' value='");
  html += manifestValue;
  html += F("' placeholder='https://raw.githubusercontent.com/usuario/repo/main/ota/manifest.txt'>");

  html += F("<label for='portal_mode'>Modo del AP</label>");
  html += F("<select id='portal_mode' name='portal_mode'>");
  html += F("<option value='always'");
  if (config.portalMode == PortalMode::AlwaysOn) {
    html += F(" selected");
  }
  html += F(">Siempre encendido</option>");
  html += F("<option value='auto'");
  if (config.portalMode == PortalMode::Auto) {
    html += F(" selected");
  }
  html += F(">Auto</option></select>");

  html += F("<button type='submit'>Guardar configuracion</button></form>");
  html += F("</div>");

  html += F("<div class='card'><h2>Acciones de red</h2><div class='actions'>");
  html += F("<form method='post' action='/wifi/connect'><button type='submit'>Conectar al Wi-Fi guardado</button></form>");
  html += F("<form method='post' action='/wifi/disconnect'><button class='secondary' type='submit'>Desconectar Wi-Fi y abrir AP</button></form>");
  html += F("<form method='post' action='/wifi/forget'><button class='warn' type='submit'>Olvidar Wi-Fi guardado</button></form>");
  html += F("<form method='post' action='/portal/force-on'><button class='secondary' type='submit'>Activar AP ahora</button></form>");
  html += F("<form method='post' action='/portal/clear-force'><button class='secondary' type='submit'>Volver al modo configurado</button></form>");
  html += F("</div>");
  html += F("<div class='warnbox'><strong>Como ubicar el ESP32:</strong> si se conecta al Wi-Fi, en esta pagina te "
            "mostramos su IP para que puedas entrar por esa red. Si preferis no depender de esa IP, dejalo en "
            "<code>Siempre encendido</code> y vas a poder entrar siempre por el AP <code>Proyecto</code>.</div>");
  html += F("</div>");

  html += F("<div class='card'><h2>OTA</h2>");
  html += F("<p>El ESP32 revisa el OTA al arrancar y luego cada 24 horas de funcionamiento.</p>");
  html += F("<form method='post' action='/ota/check'><button type='submit'>Buscar actualizacion ahora</button></form>");

  if (otaState.updateAvailable) {
    html += F("<div class='warnbox'><p><strong>Version nueva detectada:</strong> <code>");
    html += htmlEscape(otaState.remoteVersion);
    html += F("</code></p><p><strong>Notas:</strong> ");
    html += notesValue;
    html += F("</p><form method='post' action='/ota/apply'><button type='submit'>Instalar esta version</button></form></div>");
  } else {
    html += F("<p>No hay una actualizacion pendiente para instalar.</p>");
  }

  html += F("<p>Manifest recomendado:</p><pre>");
  html += htmlEscape(manifestExample);
  html += F("</pre>");
  html += F("</div>");

  html += F("</div></body></html>");
  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildPage());
}

void handleSaveConfig() {
  String incomingSsid = server.arg("wifi_ssid");
  String incomingPassword = server.arg("wifi_password");
  String incomingManifestUrl = server.arg("manifest_url");
  String incomingPortalMode = server.arg("portal_mode");

  incomingSsid.trim();
  incomingPassword.trim();
  incomingManifestUrl.trim();
  incomingPortalMode.trim();

  if (incomingSsid == config.wifiSsid && incomingPassword.isEmpty()) {
    incomingPassword = config.wifiPassword;
  }

  config.wifiSsid = incomingSsid;
  config.wifiPassword = incomingPassword;
  config.manifestUrl = incomingManifestUrl;
  config.portalMode = incomingPortalMode == "auto" ? PortalMode::Auto : PortalMode::AlwaysOn;

  saveConfig();
  refreshApState();

  setUiMessage("Configuracion guardada. Si cambiaste el Wi-Fi, toca el boton para reconectar.");
  redirectHome();
}

void handleWifiConnect() {
  if (!hasWifiCredentials()) {
    setUiMessage("Primero guarda un SSID de Wi-Fi.");
    redirectHome();
    return;
  }

  staPaused = false;
  beginStationConnection();
  setUiMessage(String("Intentando conectar a ") + config.wifiSsid + ".");
  redirectHome();
}

void handleWifiDisconnect() {
  staPaused = true;
  apManualOverride = true;
  disconnectStation(false);
  refreshApState();
  setUiMessage("Wi-Fi desconectado. El AP quedo activo para que sigas entrando al portal.");
  redirectHome();
}

void handleWifiForget() {
  staPaused = true;
  apManualOverride = true;
  config.wifiSsid = "";
  config.wifiPassword = "";
  saveConfig();
  disconnectStation(true);
  refreshApState();
  setUiMessage("Se borro el Wi-Fi guardado y el AP sigue activo.");
  redirectHome();
}

void handlePortalForceOn() {
  apManualOverride = true;
  refreshApState();
  setUiMessage("AP activado manualmente.");
  redirectHome();
}

void handlePortalClearForce() {
  apManualOverride = false;
  refreshApState();
  setUiMessage("El AP vuelve a obedecer el modo configurado.");
  redirectHome();
}

void handleOtaCheck() {
  const bool foundUpdate = checkForUpdate(true);
  if (foundUpdate) {
    setUiMessage(String("Se encontro la version ") + otaState.remoteVersion + ". Revisala y confirma si queres instalarla.");
  } else {
    setUiMessage(otaState.lastCheckMessage);
  }
  redirectHome();
}

void handleOtaApply() {
  String error;
  if (!performOta(error)) {
    otaState.lastCheckMessage = String("Fallo la instalacion OTA: ") + error;
    setUiMessage(otaState.lastCheckMessage);
    redirectHome();
    return;
  }

  server.send(200, "text/html; charset=utf-8",
              "<!doctype html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' "
              "content='width=device-width,initial-scale=1'><title>Actualizando</title></head><body>"
              "<h1>Firmware instalado</h1><p>El ESP32 va a reiniciar en unos segundos.</p></body></html>");
  delay(1500);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "Ruta no encontrada.");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config/save", HTTP_POST, handleSaveConfig);
  server.on("/wifi/connect", HTTP_POST, handleWifiConnect);
  server.on("/wifi/disconnect", HTTP_POST, handleWifiDisconnect);
  server.on("/wifi/forget", HTTP_POST, handleWifiForget);
  server.on("/portal/force-on", HTTP_POST, handlePortalForceOn);
  server.on("/portal/clear-force", HTTP_POST, handlePortalClearForce);
  server.on("/ota/check", HTTP_POST, handleOtaCheck);
  server.on("/ota/apply", HTTP_POST, handleOtaApply);
  server.onNotFound(handleNotFound);
  server.begin();
}

void updateBlink() {
  const unsigned long now = millis();
  if (now - lastBlinkMillis < AppConfig::BLINK_INTERVAL_MS) {
    return;
  }

  lastBlinkMillis = now;
  ledState = !ledState;
  applyLedOutput(ledState);
}

void maintainWifiConnection() {
  const bool connected = isStaConnected();

  if (connected && !previousStaConnected) {
    previousStaConnected = true;
    staConnecting = false;
    Serial.print(F("Wi-Fi conectado. IP: "));
    Serial.println(WiFi.localIP());
  }

  if (!connected && previousStaConnected) {
    previousStaConnected = false;
    staConnecting = false;
    Serial.println(F("Se perdio la conexion Wi-Fi."));
  }

  refreshApState();

  if (!hasWifiCredentials() || staPaused || connected) {
    return;
  }

  const unsigned long now = millis();
  if (!staConnecting || now - lastWifiAttemptMillis >= AppConfig::WIFI_RETRY_INTERVAL_MS) {
    beginStationConnection();
  }
}

void maintainOtaSchedule() {
  if (!isStaConnected()) {
    return;
  }

  const unsigned long now = millis();

  if (otaState.startupCheckPending) {
    checkForUpdate(false);
    return;
  }

  if (now - otaState.lastCheckMillis >= AppConfig::OTA_CHECK_INTERVAL_MS) {
    checkForUpdate(false);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(AppConfig::LED_PIN, OUTPUT);
  applyLedOutput(false);

  WiFi.persistent(false);
  WiFi.setSleep(false);

  loadConfig();
  startAccessPoint();

  if (hasWifiCredentials()) {
    beginStationConnection();
  }

  setupWebServer();

  Serial.println();
  Serial.println(F("ESP32 OTA iniciado."));
  Serial.print(F("Firmware: "));
  Serial.println(AppConfig::FW_VERSION);
  Serial.print(F("AP: "));
  Serial.println(AppConfig::AP_SSID);
  Serial.print(F("IP AP: "));
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  maintainWifiConnection();
  maintainOtaSchedule();
  updateBlink();
  delay(2);
}
