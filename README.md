# Panel de Control IoT - MSanchez

Interfaz web ligera para monitorizar y operar un sistema de acceso y sensores mediante MQTT sobre WebSocket. Muestra el estado de la puerta principal, sensores (temperatura, humedad, fuego, agua), ventilación y accesos NFC, y permite enviar órdenes de apertura/cierre y activación de actuadores.

## Características
- Conexión MQTT segura (wss) con credenciales configuradas en el cliente.
- Suscripción a topics de estado: puerta, sensores ambientales, ventilación y NFC.
- Envío de órdenes rápidas desde botones: abrir/cerrar puerta, activar/desactivar alarma, encender/apagar ventilación.
- Indicador de conexión y últimas lecturas en tiempo real.

## Configuración rápida
1) Abre `index.html` en un navegador con acceso al broker definido.  
2) En la sección de script ajusta, si es necesario:
   ```js
   const mqtt_host = "msanchez.ovh";
   const mqtt_port = 8444;
   const mqtt_client_id = "web_" + parseInt(Math.random() * 10000);
   const mqtt_user = "ardu";
   const mqtt_pass = "JMMAMicro";
   const mqtt_path = "/mqtt";
   ```
3) Los topics que se usan por defecto están en el array `topics` y en las llamadas `enviarOrden(...)`.

## Uso
- Al cargar la página intenta conectar y muestra el estado en la pastilla superior derecha.
- En la tarjeta "Puerta Principal" usa los botones Abrir/Cerrar para publicar en `puerta/orden`.
- En "Sistemas" envía órdenes a `alarma/orden` y `ventilacion/orden`; el estado de ventilación llega en `ventilacion/estado`.
- En "Sensores & Accesos" se visualizan las últimas lecturas y el último usuario NFC recibido en `nfc/usuario`.

## Personalización
- Estilos: variables CSS en la raíz (`:root`) controlan colores y sombras.
- Layout: las cards y grids se ajustan con CSS Grid; puedes modificar `grid-template-columns` para cambiar columnas.
- MQTT: añade o elimina topics en el array `topics` y extiende el `switch` de `onMessageArrived` para mapear nuevas lecturas.

## Seguridad
- Las credenciales MQTT están en el cliente; evita usar este archivo en producción sin moverlas a un backend o sin restricciones por IP/TLS adecuadas.
- Si cambias de broker, revisa certificados y puertos para seguir usando `useSSL: true`.
