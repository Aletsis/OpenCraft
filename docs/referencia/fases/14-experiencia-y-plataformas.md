# Fase 14 — Experiencia audiovisual, accesibilidad y plataformas

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: COMPLETADA (18 de octubre de 2026). Evidencia: [Expediente F14](../evidencias/F14/README.md). Dependencias de entrada: **F13; pruebas de viabilidad iniciadas en F00**. Brechas abordadas: **B20, B21, B22**.

## Qué lograr, por qué y para qué

Hacer que la experiencia completa pueda instalarse, entenderse y jugarse en los dispositivos objetivo. La calidad móvil se mide durante sesiones reales, no por aceptar un evento táctil.

## Alcance y límites

Menús/configuración, audio espacial, música/partículas/animación, tutorial, accesibilidad, localización, mando y controles táctiles completos. Distribuciones Linux escritorio/móvil, Android e iOS verificadas.

## Pasos de implementación

1. Unificar diseño de menú principal, mundos, pausa, opciones y multijugador; remapeo, sensibilidad, distancia de render/simulación y perfiles de calidad. Guardar configuración por dispositivo sin alterar las reglas del mundo.

2. Completar controles táctiles: mover, mirar, saltar, agacharse, romper, colocar, usar, inventario y combate simultáneos. Resolver cancelaciones de dedos, notch/área segura, DPI, rotación y tamaños compartidos entre render y hitbox.

3. Agregar audio espacial, mezcladores por categoría, subtítulos de señales relevantes, animaciones y partículas con límites. Crear recursos originales y registrar procedencia/licencias.

4. Incorporar escalado de UI, contraste, remapeo, opciones de movimiento de cámara, navegación por mando y textos español/inglés. Tutorial práctico de recoger/fabricar/refugio/guardar.

5. Preparar instalación Linux con recursos localizables fuera del cwd, integración de escritorio y paquetes ARM64. Añadir proyecto Android/NDK y proyecto/toolchain iOS; resolver backend gráfico y ciclo de vida según pruebas F00.

6. Probar suspensión, reanudación, pérdida de contexto, interrupción de audio, rotación y permisos/red local. Al ir a segundo plano guardar un estado coherente y desconectar/reconectar de forma explícita.

7. Medir sesiones de 30 minutos y optimizar según perfiles: asignaciones, mallas, IA, distancia y temperatura. No recortar reglas silenciosamente para cumplir FPS; ofrecer calidad visual/distancia configurable.

## Entregables y estado al finalizar

Paquetes instalables por plataforma declarada, manual de controles, recursos con procedencia, informe de accesibilidad y benchmarks reproducibles. Matriz exacta de sistemas/dispositivos soportados y pendientes.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Dispositivo táctil sin teclado | Crear mundo, fabricar, combatir, morir, cargar y entrar a LAN | Todo el recorrido es posible sin periféricos ni controles inaccesibles. |
| Instalación limpia fuera del repositorio | Abrir desde lanzador y rutas distintas | Recursos/configuración/guardados se encuentran correctamente. |
| Aplicación suspendida en guardado o sesión | Reanudar, rotar y recuperar contexto 20 veces | Sin corrupción, controles pegados ni duplicación de estado. |
| Hardware de referencia con escena F00 ampliada | Jugar 30 minutos tras calentamiento | Cumple métricas del plan maestro; reporta p95/p99, memoria y degradación térmica. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: aplazar incompatibilidades de plataforma hasta el final. Los prototipos F00 son obligatorios. Si falta hardware o firma de plataforma, registrar bloqueo y no declarar esa plataforma terminada; publicar solo artefactos verificados.

## Puerta de salida
 
- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias ([Expediente F14](../evidencias/F14/README.md)).
- [x] La regresión de fases anteriores afectadas sigue pasando (61/61 suites CTest, 100% éxito).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase es la **Fase 15 (Cierre de paridad, catálogo exhaustivo y candidato estable)**.
