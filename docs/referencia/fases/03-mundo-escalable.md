# Fase 03 — Streaming y generación escalable

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: completada e implementada. Dependencias de entrada: **F02**. Brechas: **B03, B11, B22**. Evidencias: [Expediente F03](../evidencias/F03/README.md).

## Qué lograr, por qué y para qué

Permitir explorar más allá de la región inicial sin que memoria y pausas crezcan con la distancia recorrida. La generación debe ser repetible y el mundo editado debe sobrevivir a la descarga.

## Alcance y límites

Semilla configurable, generación versionada, secciones verticales, carga/descarga, trabajos CPU, mallas y presupuestos. La diversidad completa de biomas y estructuras pertenece a F10.

## Pasos de implementación

1. Reemplazar las columnas monolíticas por secciones de 16³ con paletas y almacenamiento vacío compacto. Proponer rango vertical -64 a 319 para el mundo superficial de referencia; los límites serán parámetros de dimensión.

2. Introducir generador determinista por semilla, dimensión, coordenadas y versión. Usar aritmética y PRNG especificados para no depender del orden de ejecución ni de funciones trigonométricas de plataforma.

3. Implementar máquina de estados de chunk: solicitado, cargando/generando, listo, mallando, visible y descargando. Separar distancia de simulación de distancia de render y priorizar alrededor del jugador.

4. Enviar generación y mallas CPU a trabajadores; reservar GPU para el hilo de render. Versionar resultados para descartar una malla obsoleta después de una edición y cancelar trabajos de chunks abandonados.

5. Guardar antes de descargar, aplicar límites a colas/memoria y usar snapshots de vecinos para bordes. Un chunk desconocido no debe tratarse como un vacío transitable: bloquear o esperar con indicación de carga.

6. Agregar frustum culling y medición por sección; evaluar greedy meshing solo con perfiles. Para coordenadas lejanas usar enteros de mundo y posiciones de render relativas al origen.

7. Definir migración de chunks antiguos, persistir versión de generador y probar uniones. La generación de contenido posterior no debe modificar regiones ya exploradas.

## Entregables y estado al finalizar

Exploración continua, semillas reproducibles, caché acotada, generador versionado y panel de métricas. Escenas de bordes y banco de pruebas de streaming.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Dos procesos y distinto orden de tareas | Generar las mismas 100 secciones desde la misma semilla | Hashes idénticos de bloques; repetir con dos semillas produce mundos distintos. |
| Diez chunks editados | Alejarse hasta descargarlos, reiniciar y volver | Todas las modificaciones permanecen y las costuras son coherentes. |
| Recorrido de 10.000 bloques durante al menos 30 minutos | Medir residencia y colas con radio fijo | Memoria se estabiliza dentro del presupuesto F00; no crece proporcionalmente al total explorado. |
| Chunks lentos y ediciones simultáneas | Retrasar trabajadores y cruzar bordes negativos/verticales | Sin caída al vacío por datos pendientes, carreras de memoria ni mallas obsoletas visibles persistentes. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: multiplicar memoria al aumentar altura. Condicionar el rango a secciones dispersas y medir antes de agregar biomas. No prometer mundo infinito: documentar límite numérico y comportamiento al alcanzarlo.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
