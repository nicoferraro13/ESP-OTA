# ESP32 OTA - Proyecto Base

Este proyecto sirve como base para un `ESP32 DevKit V1 / ESP-WROOM-32` programado con `PlatformIO` en `Visual Studio Code`.

La idea del proyecto es:

- Hacer parpadear el LED onboard del ESP32.
- Crear un `Access Point` llamado `Proyecto` con clave `12345678`.
- Mostrar una pagina web de configuracion.
- Permitir cargar desde esa web:
  - el SSID y password de una red Wi-Fi,
  - la URL del archivo `manifest.txt` de OTA,
  - el modo de funcionamiento del AP.
- Revisar actualizaciones OTA al arrancar y luego cada 24 horas de encendido.
- Mostrar si hay una nueva version disponible.
- Esperar tu confirmacion desde la web antes de instalar la actualizacion.

## 1. Estado actual del proyecto

Hoy este proyecto ya tiene:

- configuracion de `PlatformIO`,
- tabla de particiones preparada para OTA,
- pagina web simple de administracion,
- guardado de configuracion en memoria no volatil,
- AP configurable,
- conexion opcional a Wi-Fi externo,
- revision OTA usando un `manifest.txt`,
- actualizacion OTA descargando `firmware.bin` desde GitHub.

## 2. Estructura del proyecto

Estos son los archivos mas importantes:

- `platformio.ini`
  - Configura la placa, la velocidad del monitor serie y la version del firmware.
- `partitions_ota.csv`
  - Define las particiones necesarias para poder hacer OTA.
- `include/AppConfig.h`
  - Guarda constantes del proyecto, como el pin del LED y el nombre del AP.
- `src/main.cpp`
  - Tiene toda la logica principal.
- `ota/manifest.txt`
  - Archivo de texto que le dice al ESP32 que version existe y desde donde descargarla.
- `README.md`
  - Este archivo que explica todo el flujo.

## 3. Requisitos antes de empezar

Para trabajar con este proyecto necesitas:

- Una placa `ESP32 DevKit V1` con modulo `ESP-WROOM-32`.
- Un cable USB que sirva para datos.
- Una PC con Windows.
- `Visual Studio Code`.
- La extension `PlatformIO IDE`.
- Conexion a Internet para usar GitHub y para que el ESP32 pueda descargar el firmware.

Tambien puede hacer falta el driver del chip USB a serie de tu placa. Muchas placas ESP32 usan uno de estos:

- `CP210x`
- `CH340`

Si conectas la placa y Windows no la reconoce o no aparece ningun puerto `COM`, probablemente te falta ese driver.

## 4. Como abrir este proyecto en Visual Studio Code

1. Abri `Visual Studio Code`.
2. Hace clic en `File`.
3. Hace clic en `Open Folder`.
4. Elegi esta carpeta:

```text
C:\Users\nferr\OneDrive\Documentos\Nico\Codex\ESP-OTA
```

5. Espera a que `PlatformIO` termine de reconocer el proyecto.

Si es la primera vez que abris un proyecto de `PlatformIO`, puede tardar unos minutos mientras descarga herramientas.

## 5. Como conectar la placa por USB

1. Conecta el ESP32 a la PC con el cable USB.
2. Espera unos segundos.
3. Abri el `Administrador de dispositivos` de Windows.
4. Busca la seccion `Puertos (COM y LPT)`.
5. Anota el nombre del puerto, por ejemplo:

```text
USB-SERIAL CH340 (COM5)
```

o

```text
Silicon Labs CP210x USB to UART Bridge (COM4)
```

Ese numero `COM` te sirve para identificar la placa.

## 6. Como compilar el proyecto

1. Dentro de `Visual Studio Code`, abre el proyecto.
2. En la barra inferior de `PlatformIO`, hace clic en `Build`.
3. Espera a que termine la compilacion.

Si todo esta bien, `PlatformIO` va a generar el firmware dentro de:

```text
.pio\build\esp32devkitv1\firmware.bin
```

Ese archivo es el que despues vas a usar para OTA.

## 7. Como grabar el ESP32 por primera vez usando USB

La primera carga del firmware se hace por cable. La OTA viene despues.

1. Asegurate de que la placa este conectada por USB.
2. En `Visual Studio Code`, hace clic en `Upload`.
3. Espera a que termine la grabacion.

Si la carga falla, prueba esto:

1. Mantene presionado el boton `BOOT` del ESP32.
2. Hace clic en `Upload`.
3. Cuando en la consola aparezca algo parecido a `Connecting...`, suelta el boton `BOOT`.

No todas las placas lo necesitan, pero es un problema muy comun en algunas versiones del DevKit.

## 8. Como abrir el monitor serie

1. En `PlatformIO`, hace clic en `Monitor`.
2. La velocidad de este proyecto ya esta configurada en `115200`.
3. Al arrancar deberias ver mensajes similares a estos:

```text
ESP32 OTA iniciado.
Firmware: 0.1.0
AP: Proyecto
IP AP: 192.168.4.1
```

Eso significa que el ESP32 ya arranco bien y que el AP esta activo.

## 9. Como conectarte al Access Point del ESP32

1. Desde tu celular, notebook o PC, busca redes Wi-Fi disponibles.
2. Busca la red:

```text
Proyecto
```

3. Conectate usando la clave:

```text
12345678
```

4. Abri un navegador.
5. En la barra de direcciones escribe:

```text
http://192.168.4.1
```

6. Presiona `Enter`.

Vas a ver la web local del ESP32.

## 10. Que vas a ver en la web

La pagina te muestra:

- version actual del firmware,
- si el AP esta activo o no,
- estado de la conexion Wi-Fi,
- IP del AP,
- IP obtenida en la red Wi-Fi externa, si ya conecto,
- mensaje de la ultima revision OTA,
- formulario para guardar SSID, password y URL del manifest,
- botones para controlar la red,
- botones para revisar e instalar OTA.

## 11. Como conectar el ESP32 a tu Wi-Fi de casa

1. Entra a la pagina del ESP32.
2. En el campo `SSID del Wi-Fi`, escribe el nombre exacto de tu red.
3. En el campo `Password del Wi-Fi`, escribe la clave de esa red.
4. En `Modo del AP`, elige una opcion:

- `Siempre encendido`
  - El AP `Proyecto` sigue activo todo el tiempo.
- `Auto`
  - El AP puede apagarse cuando el ESP32 este bien conectado y volver si se cae la red.

5. Hace clic en `Guardar configuracion`.
6. Despues hace clic en `Conectar al Wi-Fi guardado`.

Si conecta correctamente, la web te va a mostrar la IP que el router le dio al ESP32.

Ejemplo:

```text
IP en la red Wi-Fi: 192.168.1.35
```

Con esa IP vas a poder entrar al ESP32 desde cualquier equipo conectado a la misma red usando:

```text
http://192.168.1.35
```

## 12. Como manejar el AP desde la web

Este proyecto no te deja atrapado en una sola configuracion.

Los botones disponibles hacen esto:

- `Conectar al Wi-Fi guardado`
  - Intenta conectarse al Wi-Fi que tengas guardado.
- `Desconectar Wi-Fi y abrir AP`
  - Corta la conexion actual y deja el AP activo para que sigas entrando al portal.
- `Olvidar Wi-Fi guardado`
  - Borra SSID y password guardados.
- `Activar AP ahora`
  - Fuerza el AP aunque el equipo este conectado a otra red.
- `Volver al modo configurado`
  - Quita la fuerza manual y vuelve al comportamiento normal.

## 13. Como funciona la OTA en este proyecto

La OTA de este proyecto funciona de una manera simple:

1. El ESP32 descarga un archivo chico llamado `manifest.txt`.
2. Ese archivo le dice cual es la version disponible.
3. Tambien le dice desde que URL descargar el `firmware.bin`.
4. Si la version del manifest es distinta a la que tiene el ESP32:
   - la web muestra que hay una nueva version.
5. El ESP32 no se actualiza solo.
6. Vos tenes que confirmar la instalacion desde la web.

## 14. Formato del manifest OTA

El archivo `ota/manifest.txt` usa un formato liviano `clave=valor`.

Ejemplo:

```text
version=0.1.0
bin_url=https://raw.githubusercontent.com/USUARIO/REPO/main/ota/firmware.bin
notes=Primer firmware OTA de ejemplo
```

Campos:

- `version`
  - Version que el ESP32 va a comparar con la suya.
- `bin_url`
  - URL completa al archivo `firmware.bin`.
- `notes`
  - Texto opcional para mostrar en la web.

## 15. Como crear el repositorio en GitHub

Este proyecto ya fue inicializado como repositorio Git local en tu PC.

Ahora falta crear el repositorio remoto en GitHub.

### Paso 1: crear el repo en la web de GitHub

1. Entra a [https://github.com](https://github.com).
2. Inicia sesion con tu cuenta.
3. Hace clic en el simbolo `+` arriba a la derecha.
4. Hace clic en `New repository`.
5. En `Repository name`, escribe un nombre. Por ejemplo:

```text
ESP-OTA
```

6. Elige `Public`, porque este proyecto OTA esta pensado para repo publico.
7. No marques opciones para crear `README`, `.gitignore` ni licencia desde GitHub, porque esos archivos ya existen aca.
8. Hace clic en `Create repository`.

### Paso 2: vincular este repo local con GitHub

GitHub te va a mostrar una URL. Puede ser una de estas:

```text
https://github.com/TU-USUARIO/ESP-OTA.git
```

o

```text
git@github.com:TU-USUARIO/ESP-OTA.git
```

Si no estas acostumbrado a Git, te conviene usar la URL `https`.

En este proyecto ya dejamos configurado el repo real en GitHub:

```text
https://github.com/nicoferraro13/ESP-OTA
```

### Paso 3: agregar el remoto

Abre una terminal dentro de la carpeta del proyecto y ejecuta:

```powershell
git remote add origin https://github.com/TU-USUARIO/ESP-OTA.git
```

Para comprobar que quedo bien:

```powershell
git remote -v
```

## 16. Como hacer el primer commit y subir todo a GitHub

1. Abre una terminal dentro de la carpeta del proyecto.
2. Ejecuta:

```powershell
git add .
```

3. Luego ejecuta:

```powershell
git commit -m "Proyecto base ESP32 OTA"
```

4. Luego ejecuta:

```powershell
git push -u origin main
```

Si Git te pide usuario o clave, sigue el asistente que te muestre GitHub Desktop, el navegador o el propio Git Credential Manager.

## 17. Como dejar lista la URL del manifest

Cuando el repo ya este subido a GitHub:

1. Entra a tu repo.
2. Abre el archivo:

```text
ota/manifest.txt
```

3. Hace clic en `Raw`.
4. Copia la URL que aparece en el navegador.

En este proyecto la URL real es:

```text
https://raw.githubusercontent.com/nicoferraro13/ESP-OTA/main/ota/manifest.txt
```

Esa es la URL que despues vas a pegar en la web del ESP32.

## 18. Primera configuracion OTA real

Una vez que el repo ya esta en GitHub:

1. Entra a `http://192.168.4.1` o a la IP que tenga el ESP32 en tu red.
2. Completa:
   - SSID del Wi-Fi
   - password del Wi-Fi
   - URL del `manifest.txt`
3. Guarda la configuracion.
4. Haz clic en `Conectar al Wi-Fi guardado`.
5. Cuando ya tenga Internet, haz clic en `Buscar actualizacion ahora`.

Si la version del manifest es igual a la del firmware instalado, no va a actualizar.

Eso es correcto.

## 19. Como hacer futuras actualizaciones OTA

Cada vez que quieras publicar una nueva version, sigue exactamente este orden.

Tambien puedes automatizar casi todo con el script:

```powershell
.\scripts\publish-ota.ps1 -Build -Notes "Descripcion corta"
```

Ese script:

- compila el proyecto si usas `-Build`,
- copia `.pio\build\esp32devkitv1\firmware.bin` a `ota\firmware.bin`,
- actualiza `ota\manifest.txt` con la version actual,
- deja lista la carpeta `ota` para hacer `git add`, `git commit` y `git push`.

### Paso 1: modificar tu codigo

Haz los cambios que quieras en `src/main.cpp` o en los archivos que corresponda.

### Paso 2: cambiar la version del firmware

Abre `platformio.ini` y busca esta linea:

```ini
-D APP_VERSION=\"0.1.0\"
```

Cambia el numero de version. Por ejemplo:

```ini
-D APP_VERSION=\"0.1.1\"
```

Es importante cambiar la version.

Si no cambias la version, el ESP32 va a pensar que sigue siendo el mismo firmware.

### Paso 3: compilar

Compila otra vez el proyecto con `Build`.

Al terminar, se actualiza este archivo:

```text
.pio\build\esp32devkitv1\firmware.bin
```

### Paso 4: copiar el firmware a la carpeta OTA del repo

Toma el archivo compilado:

```text
.pio\build\esp32devkitv1\firmware.bin
```

y copialo sobre:

```text
ota\firmware.bin
```

Si `ota\firmware.bin` ya existia, reemplazalo.

Si prefieres evitar este paso manual, usa:

```powershell
.\scripts\publish-ota.ps1
```

### Paso 5: actualizar el manifest

Abre:

```text
ota\manifest.txt
```

Actualiza al menos estas lineas:

```text
version=0.1.1
bin_url=https://raw.githubusercontent.com/nicoferraro13/ESP-OTA/main/ota/firmware.bin
notes=Descripcion corta de los cambios
```

Detalles importantes:

- `version` tiene que coincidir con la nueva version del firmware.
- `bin_url` tiene que apuntar al archivo real dentro de tu repo.
- `notes` puede ser cualquier descripcion corta.

### Paso 6: subir la nueva version a GitHub

En la terminal:

```powershell
git add .
git commit -m "Publica OTA 0.1.1"
git push
```

Cuando termina el `push`, GitHub ya tiene:

- el nuevo `manifest.txt`
- el nuevo `firmware.bin`

### Paso 7: pedirle al ESP32 que revise

Entra a la pagina web del ESP32 y hace clic en:

```text
Buscar actualizacion ahora
```

Si detecta una version distinta:

- te la va a mostrar,
- te va a mostrar las notas,
- y te va a dejar confirmar la instalacion.

### Paso 8: confirmar la actualizacion

Haz clic en:

```text
Instalar esta version
```

El ESP32:

1. descarga el `firmware.bin`,
2. lo escribe en la particion OTA,
3. reinicia,
4. arranca con la nueva version.

## 20. Que pasa si algo sale mal

### Caso 1: el ESP32 no conecta al Wi-Fi

Posibles causas:

- SSID mal escrito
- password incorrecta
- la red esta muy lejos
- el router tiene filtros

Que hacer:

1. Entra al AP `Proyecto`.
2. Abre `http://192.168.4.1`.
3. Corrige SSID o password.
4. Guarda.
5. Vuelve a pulsar `Conectar al Wi-Fi guardado`.

### Caso 2: el ESP32 no encuentra la actualizacion

Posibles causas:

- la URL del manifest esta mal,
- el repo no es publico,
- el manifest no tiene el formato correcto,
- la version no cambio.

Que hacer:

1. Abre la URL del manifest en el navegador de tu PC.
2. Verifica que cargue sin pedir login.
3. Revisa que tenga `version=` y `bin_url=`.
4. Comprueba que la `version` sea distinta a la del firmware actual.

### Caso 3: el firmware no se instala

Posibles causas:

- la URL del `firmware.bin` esta mal,
- el archivo no existe en GitHub,
- el binario no corresponde a este proyecto,
- hubo corte de conexion al descargar.

Que hacer:

1. Abre la URL del `firmware.bin` en el navegador.
2. Comprueba que descargue.
3. Verifica que fue generado desde este mismo proyecto.
4. Intenta de nuevo.

## 21. Limitaciones actuales de esta version

Esta primera version fue hecha buscando simplicidad y bajo consumo de recursos de desarrollo.

Por eso:

- usa una sola pagina web simple,
- usa `HTTPS` sin validacion estricta de certificado,
- compara versiones como texto simple,
- no usa autenticacion en la web,
- no usa hash del firmware,
- no usa JSON para el manifest,
- no usa sistema de archivos para la pagina web.

Esto esta bien para un proyecto casero y para aprender.

## 22. Mejoras recomendadas a futuro

Cuando esta base ya te funcione bien, las mejoras mas utiles son:

- agregar validacion por `SHA-256`,
- pedir una clave para entrar a la web del ESP32,
- mover el HTML fuera del `main.cpp`,
- separar el codigo en varios archivos,
- automatizar la copia de `firmware.bin`,
- automatizar la publicacion con `GitHub Actions`.

## 23. Comandos Git mas utiles para este proyecto

Ver estado del repo:

```powershell
git status
```

Ver historial de commits:

```powershell
git log --oneline
```

Ver archivos modificados:

```powershell
git diff
```

Subir cambios:

```powershell
git add .
git commit -m "Describe tu cambio"
git push
```

## 24. Resumen corto del flujo normal de trabajo

Tu flujo habitual va a ser este:

1. Cambias el codigo.
2. Cambias la version en `platformio.ini`.
3. Compilas.
4. Copias `.pio\build\esp32devkitv1\firmware.bin` a `ota\firmware.bin`.
5. Editas `ota\manifest.txt`.
6. Haces `git add`, `git commit` y `git push`.
7. Desde la web del ESP32 pulsas `Buscar actualizacion ahora`.
8. Confirmas la instalacion.

## 25. Prueba OTA real recomendada

Para hacer una primera prueba real y sencilla, sigue este flujo:

1. Sube este proyecto base al repo `nicoferraro13/ESP-OTA`.
2. Compila el firmware actual.
3. Ejecuta:

```powershell
.\scripts\publish-ota.ps1
```

4. Haz:

```powershell
git add ota/firmware.bin ota/manifest.txt
git commit -m "Publica OTA inicial 0.1.0"
git push
```

5. Graba el ESP32 por USB con esta misma version `0.1.0`.
6. Entra al portal web del ESP32.
7. Carga tu Wi-Fi.
8. Carga esta URL exacta del manifest:

```text
https://raw.githubusercontent.com/nicoferraro13/ESP-OTA/main/ota/manifest.txt
```

9. Pulsa `Buscar actualizacion ahora`.

Como el ESP32 ya tiene la version `0.1.0` y el manifest tambien dice `0.1.0`, no deberia ofrecer update.

Eso es correcto y sirve para validar:

- que el ESP32 tiene Internet,
- que puede leer el manifest,
- que la URL es correcta,
- que la comparacion de version funciona.

10. Luego cambia `platformio.ini` de `0.1.0` a `0.1.1`.
11. Haz un cambio visible en el codigo, por ejemplo cambiar la velocidad de blink.
12. Compila otra vez.
13. Ejecuta:

```powershell
.\scripts\publish-ota.ps1 -Notes "Blink mas rapido para prueba OTA"
```

14. Haz:

```powershell
git add .
git commit -m "Publica OTA 0.1.1"
git push
```

15. Vuelve al portal del ESP32.
16. Pulsa `Buscar actualizacion ahora`.
17. Ahora si deberia mostrar una version nueva.
18. Pulsa `Instalar esta version`.
19. Espera el reinicio.
20. Verifica que el comportamiento cambio de verdad.

## 26. Archivos que conviene revisar primero si quieres entender el proyecto

Si quieres leer el codigo de a poco, este es un buen orden:

1. `platformio.ini`
2. `include/AppConfig.h`
3. `src/main.cpp`
4. `ota/manifest.txt`

## 27. Nota final importante

La primera grabacion del ESP32 siempre la haces por USB.

La OTA no reemplaza esa primera carga.

La OTA sirve para todas las versiones siguientes, una vez que el firmware base ya esta grabado y funcionando.
