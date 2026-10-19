# Registro de Diferencias Conocidas e Intencionales con la versión 1.21 del juego de referencia

OpenCraft usa la versión 1.21 del juego de referencia como guía de diseño para sus reglas de juego, con implementación, código y recursos propios. No obstante, ciertas decisiones de arquitectura e implementación difieren deliberadamente de la versión Java oficial con el fin de maximizar el rendimiento, la estabilidad determinista y la seguridad.

---

## 1. Arquitectura y Rendimiento

| Aspecto | Juego de referencia (1.21) | OpenCraft (C++20) | Justificación de Diseño |
|---|---|---|---|
| **Lenguaje y Runtime** | Java 21 / JVM con Garbage Collector (pausas de recolección stop-the-world). | C++20 nativo compilado con cero pausas de GC y asignación contigua orientada a datos (DoD). | Garantizar framerates estables a 60/120+ FPS y 20 TPS sin micro-tirones inducidos por recolección de memoria. |
| **Pila Gráfica** | OpenGL legacy / shaders personalizados (LWJGL). | OpenGL ES 3.0 con atlas procedural, luz suave por vértice, oclusión ambiental por vértice y descarte por frustum. | Soporte amplio para hardware embebido, Linux de escritorio y móvil y drivers libres Mesa. |
| **Multihilo en Servidor** | Bucle de simulación mayoritariamente monohilo atado al tick global. | Bucle principal a 20 TPS deterministas desacoplado del streaming asíncrono de chunks en hilos de trabajo independientes. | Escalabilidad limpia en CPUs multinúcleo modernas sin desincronización de estado lógico. |

---

## 2. Almacenamiento y Persistencia

| Aspecto | Juego de referencia (1.21) | OpenCraft (C++20) | Justificación de Diseño |
|---|---|---|---|
| **Formato de Región** | Anvil NBT comprimido con zlib/lz4 (`.mca`). | Un archivo binario por chunk con CRC32 (detección de errores, no integridad criptográfica), transacciones con journal y `fsync` de archivos y directorios, esquema versionado. Sin compresión. | Recuperación ante cierres forzados y cortes de energía: la transacción se completa o se descarta al reabrir. No se ha verificado con cortes de energía reales. |
| **Migración de Mundos** | Conversión implícita de DataFixerUpper (DFU). | Lectura compatible hacia atrás de los esquemas anteriores y copia de seguridad completa antes de reescribir un mundo antiguo. | Poder volver a la copia si una actualización de formato fallara. |

---

## 3. Red y Multijugador

| Aspecto | Juego de referencia (1.21) | OpenCraft (C++20) | Justificación de Diseño |
|---|---|---|---|
| **Protocolo de Red** | Protocolo propietario variable entre snapshots y versiones menores. | Protocolo binario UDP con canal fiable, validación de tamaño de paquetes, alcance (`reach <= 5.0m`), velocidad máxima de movimiento y remitente. | Rechazar paquetes malformados, teletransportes y acciones fuera de alcance. El cliente gráfico aún no se conecta a partidas remotas (ver `docs/ESTADO_RC1.md`). |
| **Sincronización de Paquetes de Contenido** | Resource packs transferidos por HTTP o hash SHA-1 de cliente. | `ContentPackManager` con validación de grafo de dependencias y verificación estricta de hash de red de sesión. | Prevenir discrepancias de reglas y recetas entre servidor y clientes conectados. |

---

## 4. Diferencias Menores de Jugabilidad

- **Cinabrio Quasi-Connectivity:** OpenCraft implementa la propagación de cinabrio canónica directa y retardos exactos por componente (1 tick de cinabrio = 2 ticks de juego). No se emulan comportamientos de actualización de bloque no deterministas o bugs específicos del orden de actualización por iteración de mapa hash de Java.
- **RNG y Semillas:** OpenCraft utiliza un generador pseudoaleatorio determinista de 64 bits diseñado para producir el mismo mundo en cualquier plataforma (verificado en x86_64; ARM64 sin verificar).
