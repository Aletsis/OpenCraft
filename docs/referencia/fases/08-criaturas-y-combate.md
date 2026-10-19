# Fase 08 — Criaturas, IA y combate

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: propuesta, no implementada por esta documentación. Dependencias de entrada: **F07**. Brechas: **B09**.

## Qué lograr, por qué y para qué

Crear amenazas y vida que respondan al mundo. Completa la primera noche de supervivencia y aporta recompensas con las que crecerán exploración y progresión.

## Alcance y límites

Ciclo de vida de entidades, movimiento, IA, spawn/despawn, modelos/animaciones, combate cuerpo a cuerpo y proyectiles. Primer lote: pasiva, hostil cercana, hostil a distancia y explosiva; el catálogo restante se cierra en F15.

## Pasos de implementación

1. Extender entidades F01 con salud, colisión, equipo, controlador IA y render desacoplado. Usar IDs estables y eliminar entidades mediante eventos seguros.

2. Implementar máquina de estados o árboles simples: reposo, deambular, buscar objetivo, perseguir, atacar y huir. Navegación por regiones activas, invalidación al editar terreno y presupuesto de rutas.

3. Crear reglas de aparición por dimensión, bioma, luz, superficie, dificultad y distancia; límites por familia y política de despawn persistente para mascotas/entidades nombradas.

4. Implementar daño, empuje, invulnerabilidad, enfriamiento de ataques, armadura, escudo y proyectiles. Resolver impactos en servidor lógico y evitar colisiones a través de muros.

5. Agregar botín y experiencia como entidades/eventos transaccionales. Preparar domesticación/cría para F10 y efectos avanzados para F12.

6. Crear modelos y animaciones propios con señales legibles de ataque; conectar sonidos provisionales y pruebas de accesibilidad visual. Guardar criaturas persistentes y reconstruir IA al cargar.

7. Integrar escenario completo: recolectar, fabricar, refugiarse, iluminar, sobrevivir una noche, combatir y recuperarse de una muerte.

## Entregables y estado al finalizar

Sistema de criaturas extensible, cuatro arquetipos jugables, combate y primera noche completa. Configuración de aparición y pruebas de navegación, daño y recompensas.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Enemigo tras una pared con abertura | Abrir y cerrar el camino durante persecución | Recalcula ruta; no atraviesa sólidos ni queda consumiendo CPU sin límite. |
| Habitaciones oscuras/iluminadas en dificultad fija | Ejecutar simulación con semilla y reloj controlados | Apariciones respetan condiciones y límites declarados. |
| Dos impactos letales en el mismo tick | Procesar muerte, guardar y cargar | Una muerte, un botín y experiencia única. |
| Escena con 100 entidades activas como objetivo inicial | Simular diez minutos y descargar regiones | Respeta presupuesto de tick; entidades persistentes vuelven sin duplicarse. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: IA cara antes de medir. Escalonar percepción y rutas sin alterar reglas críticas. Los 100 agentes son un objetivo de prueba OpenCraft, no una garantía de equivalencia con el juego de referencia.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias ([evidencias/F08/README.md](../evidencias/F08/README.md)).
- [x] La regresión de fases anteriores afectadas sigue pasando (36/36 tests CTest pasando al 100%).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (Esquema v7 con persistencia de criaturas por chunk, armadura y XP).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.

