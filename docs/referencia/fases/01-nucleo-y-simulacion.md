# Fase 01 — Núcleo de simulación y modelo de datos

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: implementada (expediente en [evidencias/F01](../evidencias/F01/README.md)). Dependencias de entrada: **F00**. Brechas: **B01, B08, B10, B22**.

## Qué lograr, por qué y para qué

Dar a mundo, inventario, IA y red una única forma de cambiar estado. Esto evita reescribir cada mecánica al añadir multijugador y permite pruebas sin GPU.

## Alcance y límites

Separación de aplicación, simulación y render; tick fijo, comandos de juego, registros de contenido, estados de bloque e identidad de entidades. Contención de fallos de red conocidos. No reemplazar todo el motor ni construir una abstracción sin un uso probado.

## Pasos de implementación

1. Extraer de `src/main.cpp` un controlador de aplicación y una sesión de mundo. La entrada emite intenciones y el render consulta snapshots; `World` deja de depender de `Camera` para la lógica.

2. Introducir un reloj de simulación de 20 ticks/s como propuesta inicial, acumulador e interpolación visual. Limitar recuperación tras pausas sin ejecutar colas ilimitadas. Definir orden estable de sistemas.

3. Sustituir la identidad persistente basada en `uint8_t BlockID` por claves estables con namespace y registros de estados. Usar IDs compactos internos sin convertir su orden en formato de disco; diseñar paletas por sección.

4. Definir `GameCommand`, resultado y eventos confirmados para romper/colocar, más `EntityId`, componentes mínimos de posición y ciclo de vida. La simulación local actúa como autoridad desde esta fase.

5. Unificar validaciones de alcance, colisión, límites y modo. Separar definición de bloque de su estado colocado y de sus datos persistentes, preparando cofres y hornos.

6. Corregir lectura de paquetes: tamaño exacto por tipo, nombres acotados, números finitos, IDs válidos y remitentes esperados. Añadir un interruptor para desactivar LAN incompleta hasta F09; no aceptar mutaciones arbitrarias de red.

7. Desacoplar acceso al disco y subida GPU mediante interfaces pequeñas; documentar propietario de memoria y de cada hilo. Mantener jugable el prototipo después de cada extracción.

## Entregables y estado al finalizar

Biblioteca de simulación sin SDL/GLES obligatorios, sesión local autoritativa, registros versionados, adaptadores de entrada/render y pruebas de comandos. Documento de orden de ticks y contrato de eventos.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Semilla y secuencia de comandos fijadas | Reproducir 1.000 ticks con render a 30, 60 y 144 FPS | Mismo hash del estado lógico en las tres ejecuciones. |
| Registro con más de 255 estados | Guardar IDs lógicos en una fixture y cambiar el orden de registro | No se truncan estados ni cambia la identidad del contenido. |
| Paquetes truncados y tipos desconocidos | Inyectar entradas de tamaño 0 hasta el máximo permitido bajo sanitizadores | Sin lecturas fuera de límites ni mutaciones aceptadas. |
| Prototipo de construcción | Caminar, saltar, romper y colocar junto a bordes de chunk | Se conserva la funcionalidad y se emite un único evento por acción válida. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: sobreingeniería y regresiones al separar `main`. Migrar verticalmente una acción antes del resto. La autoridad completa de sesión y transporte fiable se entrega en F09, pero las reglas nunca deben regresar al render.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias (evidencias/F01/README.md).
- [x] La regresión de fases anteriores afectadas sigue pasando (suite CTest 100% aprobada).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (BlockRegistry versionado y estable).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
