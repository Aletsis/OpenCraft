# Fase 11 — Cinabrio, máquinas y transporte de objetos

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: 🟢 **COMPLETADA**. Dependencias de entrada: **F10**. Brechas: **B14 (Cerrada), B15 (Parcial ampliada)**. Evidencias: [evidencias/F11](../evidencias/F11/README.md). Hito: **H5 (Completado)**.

## Qué lograr, por qué y para qué

Permitir construir sistemas que trabajen juntos: puertas, granjas, clasificación y transporte. Es una de las áreas donde los pequeños detalles de actualización afectan a la paridad.

## Alcance y límites

Señales, cableado, emisores, lógica, detectores, actuadores, pistones, tolvas, dispensadores, crafter y rieles/vagonetas. La equivalencia de circuitos Java se verifica explícitamente; no se asume por incluir los componentes.

## Pasos de implementación

1. Fijar niveles de señal, orden de vecinos, ticks programados, pulsos y política de regiones dormidas. Enumerar diferencias Java/Bedrock y casos como cuasiconectividad en el catálogo.

2. Implementar polvo, fuente, palanca, botón, antorcha, repetidor, comparador y sensores con estados persistentes. Separar resolver señal de modificar físicamente el mundo.

3. Agregar lámparas, puertas y pistones con reglas de movimiento, colisión y bloques inmovibles. Un movimiento de varios bloques es una transacción; conservar sus estados y entidades asociadas.

4. Implementar tolvas, soltadores y dispensadores sobre las transacciones F04; tiempos, filtro por cara y comportamiento con inventario lleno definidos por datos.

5. Conectar crafter al motor F05, con ranuras y activación por pulso. Agregar rieles, rieles de activación/propulsión y vagonetas con carga persistente.

6. Construir fixtures de puerta automática, reloj, clasificador, granja y línea de fabricación. Medir circuitos oscilantes y aplicar límites de trabajo sin destruir su estado lógico.

7. Replicar estados visibles y resultados al cliente, manteniendo la simulación de circuitos exclusivamente en servidor.

## Entregables y estado al finalizar

Kit de automatización con mapa de circuitos de referencia, semántica documentada y pruebas por tick. Lista visible de comportamientos todavía distintos de Java 1.21.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Circuito lógico y reloj con salidas esperadas por tick | Ejecutar con distinto orden de carga/render | Secuencias de señal idénticas a las fixtures de referencia. |
| Pistón junto a contenedor y borde de chunk | Activar, guardar durante transición y recuperar | Ningún bloque/ítem se duplica o desaparece; estado final consistente. |
| Clasificador con entrada de 1.000 ítems | Procesar, detener y reiniciar servidor | Conservación exacta: entrada = salida + retenidos + consumos explícitos. |
| Circuito oscilante de estrés | Mantenerlo activo con clientes conectados | Servidor sigue respondiendo y respeta presupuesto; cualquier limitación de paridad se registra. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: declarar cinabrio completa con una puerta de demostración. Cada propiedad de timing/circuito del catálogo necesita un caso propio, incluidos bordes y suspensión. La paridad estricta no admite excepciones ocultas por optimización.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias ([evidencias/F11](../evidencias/F11/README.md)).
- [x] La regresión de fases anteriores afectadas sigue pasando (48/48 pruebas superadas).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (Esquema v9 compatible).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado (1.000 ítems conservados).
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
