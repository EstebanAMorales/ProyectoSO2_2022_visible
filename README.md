Sistemas Operativos II - Laboratorio II IPC
###  Ingeniería en Compitación - FCEFyN - UNC
# Interprocess Communication

## Diseño
Para realizar el práctico elegí emplear las llamadas bloqueantes de la API de sockets pero dentro de las rutinas de 
threads diferentes.
En primera instancia se crearán de manera fija 3 hilos para atender a las solicitudes de conección para cada protocolo.
Inicialmente la solución a implementar soportará conecciones IPv4 TCP, IPv6 TCP y UNIX domain sockets con conexión.
Para servir las funcionalidades descriptas en la consigna se creará un thread distinto para manejar cada una de las conecciones aceptadas.
A saber:
Socket IPv4 TCP: Implementará la funcionalidad del Cliente A.
Socket IPv6 TCP: Implementará la funcionalidad del Cliente B.
Socket UNIX: Implementará la funcionalidad del Cliente C.

## Implementación
Se empleó la api de manejo de hilos ofrecidas por la librería pthread. Todos los hilos creados se inicializaron como detached
para evitar problemas de sincronización con el proceso principal.

El proceso servidor crea 5 conexiones a la base de datos sqlite configuradas con el flag SQLITE_OPEN_NOMUTEX de manera que el acceso a todas las conexiones se puede realizar de manera concurrente siempre y cuando cada conexión sea accedida por un único thread a cada momento.

Al momento de recibir una solicitud de conexión el servidor asignará una de estas conexiones al thread que atenderá al nuevo cliente.
Se mantiene contadores de uso para cada conexión para asegurar una igual distribución de clientes por conexión.

Los clientes se implementaron como procesos diferentes en archivos de código fuente separados. Sin embargo, al tener implementaciones similares
se favoreció la reutilización de código mediante la creación de librerías comúnes tanto para clientes como para el servidor.


## Compilación
Para compilar y contruir los ejecutables se emplea cmake
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Ejecución

## Ejecutables del proyecto
El proyecto genera 4 ejecutables, el servidor y 3 clientes.
```
$ ./server_tp2 7799 7077 ./so2_2022_tp1.socket bw_usage_log_file.txt
```
```
$ ./unix_client ./so2_2022_tp1.socket ../fake180mbfile.fake 12
```
```
$ ./tcp6_client 7077 ../fake180mbfile.fake 12
```
```
$ ./tcp4_client 7799 ../fake20mbfile.fake 400
```
