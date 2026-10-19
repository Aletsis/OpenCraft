# Fase 07 — Luz, tiempo, clima y simulación ambiental

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: propuesta, no implementada por esta documentación. Dependencias de entrada: **F06**. Brechas: **B07, B08**.

## Qué lograr, por qué y para qué

Hacer que el entorno influya en supervivencia y construcción. Luz, fluidos y actualizaciones de bloques serán entradas de agricultura, aparición de criaturas y automatización.

## Alcance y límites

Luz de cielo/bloque, día/noche, clima, agua/lava, inmersión, fuego, caída de bloques y actualizaciones vecinas/programadas. No se exige iluminación fotorrealista.

## Pasos de implementación

1. Crear colas de actualizaciones con orden determinista y presupuesto por tick. Persistir actualizaciones pendientes; definir qué ocurre en bordes de chunks dormidos.

2. Implementar canales de luz de cielo y emisión, propagación y retirada cuando se coloca/rompe un bloque. Incluir vecinos de secciones y evitar que la falta de datos equivalga a luz plena.

3. Conectar reloj de mundo a cielo, niebla, iluminación y sueño; añadir lluvia/tormenta con estado persistente y reglas de exposición. Separar brillo visual de nivel lógico de luz.

4. Implementar estados de fluido (fuente/nivel/dirección), flujo, cubetas, interacción agua/lava y coexistencia con bloques que lo admitan. Especificar reglas finitas y renovación según catálogo.

5. Conectar volumen sumergido con natación, flotación, oxígeno y daño de lava. Añadir fuego/propagación y gravedad de arena con límites para evitar avalanchas de tareas.

6. Separar pases opaco, recortado y translúcido, ordenar transparencia y calcular AO vecinal si el perfil lo permite. Probar hojas/agua/vidrio adyacentes y no solo una textura aislada.

## Entregables y estado al finalizar

Ambiente funcional, iluminación lógica consultable por IA/cultivos, fluidos persistentes y colas observables. Mundos de prueba de luz, inundación y fuego.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Habitación cerrada y antorcha junto al borde de chunk | Colocar y quitar luz; cerrar/abrir techo | Propagación y retirada alcanzan estado correcto sin luz residual tras recarga. |
| Tanque con fuente y cubeta | Llenar, retirar fuente, descargar y regresar | Flujo coincide con reglas y actualizaciones pendientes no se pierden. |
| Jugador sumergido o en lava | Esperar ticks, salir y guardar | Aire/daño se calculan una vez y se detienen/recuperan según estado. |
| Cascada extensa, arena y fuego | Ejecutar escena de estrés 30 minutos | Colas acotadas, sin bloqueo del hilo principal; retraso observable y dentro del presupuesto fijado. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: actualizaciones recursivas infinitas y coste al retirar luz. Usar colas iterativas con deduplicación y medir convergencia. No simplificar reglas silenciosamente: anotar diferencias en catálogo.

## Puerta de salida

- [ ] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [ ] La regresión de fases anteriores afectadas sigue pasando.
- [ ] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [ ] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [ ] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
