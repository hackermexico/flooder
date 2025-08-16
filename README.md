# flooder
=== Advanced Network Stress Tool ===

Descripción
El Advanced Network Stress Tool es una herramienta de prueba de estrés de red que implementa múltiples técnicas para evaluar la resistencia de sistemas y redes a diferentes tipos de ataques. Permite simular varios vectores de ataque para identificar vulnerabilidades y puntos débiles en infraestructuras.

Advertencia: Esta herramienta debe usarse únicamente en entornos controlados y con autorización explícita. El uso no autorizado contra sistemas ajenos es ilegal y poco ético.

Características principales
8 técnicas de ataque diferentes

Soporte para múltiples hilos simultáneos

Estadísticas en tiempo real

Spoofing de direcciones IP

Generación de tráfico de red masivo

Interfaz de usuario intuitiva

Técnicas de ataque implementadas
TCP SYN Flood
Inunda el objetivo con solicitudes SYN de TCP, agotando las tablas de conexión.

UDP Flood
Envía paquetes UDP masivos con payloads aleatorios para saturar el ancho de banda.

ICMP Flood (Ping Flood)
Genera un gran volumen de solicitudes ICMP Echo Request.

HTTP Flood
Realiza múltiples solicitudes HTTP complejas con diferentes métodos y cabeceras.

Slowloris
Mantiene múltiples conexiones HTTP abiertas durante el mayor tiempo posible.

Resource Exhaustion
Consume recursos locales (CPU y memoria) para simular ataques internos.

DNS Amplification
Utiliza servidores DNS públicos para amplificar el tráfico dirigido al objetivo.

Mixed Attack
Combina aleatoriamente diferentes técnicas durante la ejecución.

Requisitos del sistema
Sistema operativo Linux

Compilador C++ compatible con C++11

Permisos de superusuario (para operaciones con sockets raw)

Bibliotecas de desarrollo de red

Compilación
bash
g++ -o stress_tool main.cpp -lpthread -O3
Uso
Compilar el programa

Ejecutar con privilegios de superusuario:

bash
sudo ./stress_tool
Seguir las indicaciones interactivas:

Ingresar IP objetivo

Especificar puerto

Definir duración del ataque (en segundos)

Seleccionar número de hilos

Elegir técnica de ataque

Estadísticas en tiempo real
Durante la ejecución, la herramienta muestra:

Paquetes totales enviados

Datos totales transmitidos (MB)

Ancho de banda utilizado (KB/s)

Estadísticas por hilo

Consideraciones legales y éticas
Este software se proporciona únicamente con fines educativos y de prueba en entornos controlados. El usuario es responsable de:

Obtener autorización explícita antes de probar cualquier sistema

Cumplir con todas las leyes locales y regulaciones de redes

No utilizar esta herramienta para actividades maliciosas

Limitar las pruebas a sistemas de su propiedad o con permiso explícito

Mejoras futuras
Implementar ataques de capa de aplicación adicionales (HTTPS, DNS)

Añadir modo de ataque distribuido

Mejorar la gestión de errores y reintentos

Implementar técnicas de evasión de IDS/IPS

Añadir soporte para IPv6

Desarrollar interfaz gráfica

Captura de pantalla
text
███████╗████████╗██████╗ ███████╗███████╗███████╗██████╗ 
██╔════╝╚══██╔══╝██╔══██╗██╔════╝██╔════╝██╔════╝██╔══██╗
███████╗   ██║   ██████╔╝█████╗  ███████╗█████╗  ██████╔╝
╚════██║   ██║   ██╔══██╗██╔══╝  ╚════██║██╔══╝  ██╔══██╗
███████║   ██║   ██║  ██║███████╗███████║███████╗██║  ██║
╚══════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝

=== Advanced Network Stress Tool ===
Métodos disponibles:
1. TCP SYN Flood
2. UDP Flood
3. ICMP Flood
4. HTTP Flood
5. Slowloris
6. Resource Exhaustion
7. DNS Amplification
8. Mixed Attack
0. Salir

Ingrese la IP objetivo: 192.168.1.100
Ingrese el puerto: 80
Duración del ataque (segundos): 60
Número de hilos: 10
Seleccione método: 4
