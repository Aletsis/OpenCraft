# Fase 10 — Biomas, estructuras y ecosistemas

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: propuesta, no implementada por esta documentación. Dependencias de entrada: **F09**. Brechas: **B11, B12, B13, B15**.

## Qué lograr, por qué y para qué

Dar motivos para viajar y establecer bases renovables. Una variedad visual debe corresponder a recursos, encuentros y sistemas útiles.

## Alcance y límites

Generación superficial/subterránea por bioma, estructuras, cultivos, pesca, cría, domesticación, aldeas/comercio y navegación básica. El primer lote no sustituye el catálogo completo de F15.

## Pasos de implementación

1. Extender generador versionado con parámetros de clima/altura y densidad 3D: cuevas, acuíferos y distribución de minerales. Definir pasadas que no dependan del orden de carga de chunks.

2. Primer lote obligatorio: llanura, bosque, desierto, frío, montaña y océano, además de cuevas; asociar vegetación, fauna y recursos. Registrar después cada bioma restante en catálogo.

3. Crear sistema de plantillas/piezas de estructuras con anclas globales y límites: primera aldea, mazmorra y ruina con botín. Una estructura que cruza chunks se genera una sola vez.

4. Implementar suelo cultivado, humedad, luz y crecimiento por ticks; semillas/cosecha y replantación. Añadir pesca con tablas de botín y condiciones de agua.

5. Agregar cría, alimento, crecimiento, domesticación y mascotas persistentes. Implementar aldeanos con camas/estaciones, profesiones, ofertas, reposición y economía verificable.

6. Añadir barcos, monturas y objetos de navegación: mapas, brújula y coordenadas según reglas elegidas. Rieles/vagonetas se completan en F11.

7. Integrar descubrimientos/avances iniciales y pruebas multijugador de granjas/comercio. Preparar estructuras especiales y eventos de catálogo para F15.

## Entregables y estado al finalizar

Mundo diverso con destinos, ciclo de alimento renovable, asentamientos y transporte básico. Informe de distribución de recursos y catálogo de biomas/estructuras cubiertos y pendientes.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Conjunto de 100 semillas fijadas | Generar muestra regional y analizar conectividad/recursos | Cada familia tiene fixtures encontrables; las rutas de progresión no dependen de recursos imposibles. |
| Estructura en cuatro chunks | Generarlos en varios órdenes y guardar | Misma estructura y botín inicial; sin duplicación por descargar/cargar. |
| Granja con condiciones válidas e inválidas | Sembrar, esperar ticks, cosechar y replantar | Produce ciclo renovable; el control sin condiciones no crece indebidamente. |
| Comerciante con existencias limitadas y dos clientes | Comprar simultáneamente y recargar mundo | Consumo, pago y stock se confirman atómicamente y mantienen límites. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: producir mundos bonitos sin progresión viable. Evaluar distribución y accesibilidad, además de capturas. No cambiar la semilla o el generador de chunks ya guardados para corregir contenido nuevo.

## Puerta de salida

- [ ] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [ ] La regresión de fases anteriores afectadas sigue pasando.
- [ ] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [ ] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [ ] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
