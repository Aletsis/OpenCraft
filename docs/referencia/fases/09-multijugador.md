# Fase 09 — Multijugador autoritativo y servidor

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: completada. Dependencias de entrada: **F08**. Brechas: **B10**.

## Qué lograr, por qué y para qué

Compartir el mismo mundo y la misma economía con consistencia. Sustituye los paquetes desconectados actuales por una sesión cooperativa verificable.

## Alcance y límites

Servidor lógico local/dedicado, descubrimiento LAN, conexión por dirección, identidad de sesión, snapshots, acciones fiables, interpolación, chat y administración básica. No incluye interoperabilidad con servidores del juego de referencia ni servicio alojado tipo Realms.

## Pasos de implementación

1. Definir protocolo versionado con serialización explícita, negociación de contenido, ID de sesión, jugadores y esquema. Completar handshake/ack, reintentos y desconexión; no confiar en structs empaquetados enviados directamente.

2. Extraer ejecutable servidor sin ventana. Singleplayer usa el mismo camino de comandos que LAN. El servidor valida alcance, inventario, daño y permisos; el cliente envía intención, no estado final de bloques.

3. Implementar entrega fiable/ordenada para inventarios, chunks y cambios críticos; posiciones pueden usar canal no fiable con secuencia. Elegir biblioteca o transporte tras una evaluación documentada de licencias, mantenimiento y coste.

4. Sincronizar snapshot al entrar, cambios durante su envío y suscripciones por interés/dimensión. Usar revisiones y reenvío/resincronización para reparar divergencias.

5. Replicar entidades y jugadores a todos los clientes relevantes, con modelos visibles, interpolación y predicción/reconciliación local. Integrar cada mecánica ya existente, no solo posición y bloques.

6. Implementar reconexión, timeouts, límites de paquetes, validación de remitente, autenticación de sesión y permisos de administración. Agregar lista LAN, conexión manual, chat, expulsión y lista de acceso.

7. Persistir identidad/inventario en servidor y definir política al perder host. No prometer migración automática de host; informar cierre y permitir reabrir el mundo guardado.

## Entregables y estado al finalizar

Cooperativo LAN real y servidor dedicado, protocolo documentado, herramienta de diagnóstico y pruebas con red degradada. Nuevos sistemas posteriores deben añadir su réplica en la misma fase que se implementen.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Host y tres clientes con versiones compatibles | Entrar después de editar mundo, mover, luchar y abrir cofres | Todos ven jugadores y comparten estado inicial y cambios posteriores. |
| Dos clientes intentan extraer el último objeto | Repetir solicitudes y desconectar a uno | Solo uno obtiene el objeto; no se pierde ni duplica tras reconectar. |
| Red con 100 ms RTT, 2 % pérdida, duplicados y reordenamiento | Jugar 30 minutos y luego dejar la red estable | Tras converger, hashes de estado replicado coinciden; sin divergencia permanente. |
| Cliente incompatible o mensaje malformado | Intentar handshake y acciones fuera de alcance | Rechazo claro, sin caída del servidor ni mutación no autorizada. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: añadir una segunda implementación de reglas para la red. Reutilizar F01; empezar por conexión loopback y después LAN. Una prueba LAN no certifica publicación segura en Internet: documentar límites y probar autenticación antes de ofrecer acceso remoto.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias ([evidencias/F09/README.md](../evidencias/F09/README.md)).
- [x] La regresión de fases anteriores afectadas sigue pasando (40/40 tests CTest pasando al 100%).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (servidor dedicado headless con auto-guardado periódico y desconexiones limpias).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado (control optimista de revisiones en cofres y validación de alcance autoritativa).
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
