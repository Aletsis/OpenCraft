> **Documento histórico de planificación.** El estado vigente de la versión 1.0.0-rc1, con lo verificado y las limitaciones conocidas, está en [ESTADO_RC1.md](../ESTADO_RC1.md).

# Plan maestro para acercar OpenCraft a la paridad con el juego de referencia

Fecha: 15 de octubre de 2026. Estado: propuesta de desarrollo sustentada en revisión estática; ninguna fase se marca implementada por crear estos documentos.

## Punto de partida y objetivo

OpenCraft cuenta con un prototipo vóxel C++20/SDL3/GLES, movimiento, construcción, terreno finito y una base LAN incompleta. Faltan los sistemas que convierten esa base en una experiencia completa: persistencia, inventario, fabricación, supervivencia, ambiente simulado, criaturas, cooperación consistente, exploración, automatización y progresión. El diagnóstico y las fuentes están en [COMPARACION.md](COMPARACION.md).

El objetivo propuesto es igualar las **capacidades y el catálogo jugable de la versión 1.21 del juego de referencia**, conservando la orientación ligera y móvil de OpenCraft. Bedrock sirve como referencia complementaria de interacción móvil. Esta elección congela una meta comprobable; no afirma que 1.21 sea la versión más reciente. La publicación oficial de Java 1.21 (notas oficiales de la versión) es el ancla de versión.

Hay tres niveles diferentes de entrega:

1. **Base técnica confiable:** explorar, construir y conservar un mundo de forma reproducible.
2. **Equivalencia jugable de sistemas:** construir, sobrevivir, cooperar, automatizar y completar progresión con contenido representativo. No significa tener todos los bloques o criaturas.
3. **Paridad de referencia:** completar el inventario exhaustivo de contenido/reglas acordado y verificar sus interacciones. F15 es obligatorio para esta afirmación; no es un simple pulido.

No forman parte de esta primera meta la compatibilidad de protocolo/archivos del juego de referencia, sus mods Java, Marketplace, Realms como servicio comercial, consolas ni cada actualización posterior a 1.21. Son líneas adicionales, no capacidades que se puedan dar por hechas. Se usarán recursos gráficos, música, modelos y código propios o con licencia adecuada; la equivalencia buscada es funcional.

## Orden de ejecución y archivos de especificaciones

Los tamaños son complejidad relativa, no semanas ni compromisos de calendario. M: cambio acotado con varios módulos; G: varios subsistemas; XL: varias entregas internas y riesgo alto. No conocemos la capacidad del equipo ni disponemos aún de benchmarks, por lo que fijar fechas sería especulativo.

| Fase | Resultado al terminar | Dependencia | Tamaño | Especificación |
| --- | --- | --- | --- | --- |
| F00 | Referencia congelada, catálogo y calidad medible | — | G | [Referencia y calidad](fases/00-referencia-y-calidad.md) |
| F01 | Simulación separada, comandos y modelo de datos estable | F00 | G | [Núcleo y simulación](fases/01-nucleo-y-simulacion.md) |
| F02 | Mundos guardables, recuperables y migrables | F01 | G | [Persistencia](fases/02-persistencia.md) |
| F03 | Exploración continua con memoria acotada | F02 | XL | [Mundo escalable](fases/03-mundo-escalable.md) |
| F04 | Inventarios y objetos sin pérdida/duplicación | F03 | G | [Inventario y objetos](fases/04-inventario-y-objetos.md) |
| F05 | Recolectar, fabricar y mejorar herramientas | F04 | G | [Fabricación y minería](fases/05-fabricacion-y-mineria.md) |
| F06 | Salud, comida, muerte, descanso y modos base | F05 | G | [Supervivencia](fases/06-supervivencia.md) |
| F07 | Luz, tiempo, clima, fluidos y bloques activos | F06 | XL | [Entorno y fluidos](fases/07-entorno-y-fluidos.md) |
| F08 | Criaturas, combate y primera noche completa | F07 | XL | [Criaturas y combate](fases/08-criaturas-y-combate.md) |
| F09 | Cooperación consistente y servidor dedicado (🟢 Completada) | F08 | XL | [Multijugador](fases/09-multijugador.md) |
| F10 | Biomas, estructuras, granjas, comercio y viajes (🟢 Completada) | F09 | XL | [Exploración y ecosistemas](fases/10-exploracion-y-ecosistemas.md) |
| F11 | Máquinas, circuitos y transporte automatizado (🟢 Completada) | F10 | XL | [Automatización](fases/11-automatizacion.md) |
| F12 | Equipo avanzado, dimensiones y jefes (🟢 Completada) | F11 | XL | [Progresión y dimensiones](fases/12-progresion-y-dimensiones.md) · [Evidencias](evidencias/F12/README.md) |
| F13 | Modos completos y herramientas de creación (🟢 Completada) | F12 | G | [Modos y creadores](fases/13-modos-y-creadores.md) · [Evidencias](evidencias/F13/README.md) |
| F14 | Experiencia terminada en plataformas verificadas (🟢 Completada) | F13 | XL | [Experiencia y plataformas](fases/14-experiencia-y-plataformas.md) · [Evidencias](evidencias/F14/README.md) |
| F15 | Catálogo completo, evidencias y candidato estable (🟢 Completada) | F00–F14 | XL | [Cierre de paridad](fases/15-cierre-de-paridad.md) · [Evidencias](evidencias/F15/README.md) |

El orden es una secuencia conservadora de integración para el estado actual, no la afirmación de que cada sistema sea técnicamente inseparable del anterior. Algunas investigaciones pueden adelantarse: plataforma en F00, protocolo en F01, arte/audio durante implementación y enumeración de catálogo desde el principio. El producto de cada fase debe mantenerse ejecutable. La red autoritativa se prepara desde F01 aunque la sesión completa se cierre en F09.

## Hitos jugables y condiciones de demostración

| Hito | Fases | Demostración obligatoria |
| --- | --- | --- |
| H0 — Base medible | F00–F01 | Compilar desde instrucciones, ejecutar lógica sin GPU y repetir comandos con igual resultado a varios FPS. |
| H1 — Mundo propio | F02–F03 | Crear mundo, construir lejos del origen, cerrar y volver a encontrar la construcción. |
| H2 — Producción | F04–F05 | Empezar vacío, obtener materiales, fabricar herramientas y fundir hierro; guardar a mitad del proceso. |
| H3 — Primera noche | F06–F08 | Buscar alimento, construir/iluminar refugio, enfrentar enemigos, morir y recuperar recursos sin duplicación. |
| H4 — Cooperación | F09 | 🟢 Completado ([evidencias/F09](evidencias/F09/README.md)). Cuatro jugadores comparten criaturas, cofres y mundo; uno entra tarde y otro reconecta. |
| H5 — Mundo productivo | F10–F11 | 🟢 Completado ([evidencias/F11](evidencias/F11/README.md)). Viajar, cultivar, comerciar y construir una granja automatizada que sigue coherente tras reiniciar. |
| H6 — Progresión completa | F12–F13 | 🟢 Completado ([evidencias/F13](evidencias/F13/README.md)). Llegar desde mundo nuevo al jefe final, volver, seguir jugando y usar otros modos y paquetes propios. |
| H7 — Producto verificable | F14–F15 | 🟢 Completado ([evidencias/F15](evidencias/F15/README.md)). Catálogo 1.21 (cámaras de desafío) completo, regresión multi-sistema, simulación 8h de servidor, migración hacia adelante esquemas v1-v11 a v12, release candidate empaquetado. |

Estos hitos no son intercambiables: una demo del jefe usando comandos no certifica la cadena de progresión; una granja de ejemplo no certifica toda la cinabrio.

## Contratos técnicos que deben mantenerse

- **Autoridad única:** entrada/UI/red envían intenciones; la simulación valida y confirma. Singleplayer y servidor comparten reglas. Inventarios, botín, comercio y portales usan transacciones.
- **Tiempo definido:** tick fijo propuesto de 20 Hz; render interpolado independiente. Relojes, IA, hornos, cultivos, fluidos y circuitos expresan tiempos en unidades de simulación.
- **Identidad estable:** IDs de contenido con namespace, registros de estados y entidades. Los índices compactos en RAM no son la identidad de disco o de paquetes.
- **Persistencia extensible:** cada sistema nuevo entrega serialización, versión/migración y fixture de recuperación en su misma fase. No posponer guardado de entidades hasta el lanzamiento.
- **Mundo bajo presupuesto:** secciones dispersas, carga/descarga, radios de simulación/render separados, colas limitadas y versión del generador por región explorada.
- **Contenido verificable:** recetas, drops, reglas y variantes en datos cuando sea útil. Cada entrada se vincula a origen, fase y prueba; una textura nueva no demuestra una mecánica.
- **Móvil desde el inicio:** interfaces de entrada por acciones, hitboxes coherentes, perfiles de recursos y pruebas tempranas de ciclo de vida. F14 completa y certifica, no descubre toda la portabilidad.

Los nombres nuevos de módulos que aparecen en fases son propuestas; los únicos módulos existentes son los citados en la comparación. No se requiere un ECS complejo ni cambiar a otro motor si interfaces sencillas resuelven el problema.

## Presupuestos iniciales de calidad

Son **metas propuestas de OpenCraft**, no mediciones realizadas ni requisitos atribuidos al juego de referencia. F00 debe escoger hardware real y confirmar o revisar estos valores con justificación. Una modificación de presupuesto queda registrada; no se cambia durante una prueba para considerarla aprobada.

| Aspecto | Meta inicial | Procedimiento |
| --- | --- | --- |
| Render escritorio | p95 de frame ≤16,7 ms a 1280×720 | Tres recorridos iguales de diez minutos, tras calentamiento; registrar p99 y pausas. |
| Render móvil base | p95 de frame ≤33,3 ms a resolución interna 1280×720 | Sesión de 30 minutos en ARM64 identificado, incluyendo tramo con carga térmica. |
| Simulación | 20 ticks/s; p95 ≤40 ms y p99 ≤50 ms | Servidor con cuatro jugadores, 100 entidades activas y mapa fijo con circuitos/granja. |
| Memoria móvil | Pico RSS objetivo ≤512 MiB en perfil base | Mundo, radio 3 de render y 2 de simulación; GPU aparte cuando sea medible. Revisar con F00/F03 antes de congelar. |
| Streaming | Memoria residente y colas acotadas | Recorrer 10.000 bloques, volver y confirmar ediciones; publicar máximos y evolución temporal. |
| Guardado | Estado confirmado recuperable; autoguardado propuesto cada 30 s | Fallos inyectados en escritura, disco lleno y reinicio. Informar ventana de progreso no confirmado. |
| Red | Cuatro jugadores; 100 ms RTT y 2 % pérdida | Secuencias repetidas/alteradas; convergencia verificable de estado tras restaurar conectividad. |
| Estabilidad | Cero corrupción, duplicación o crash en aceptación | Ocho horas de servidor en F15 y ciclos móviles de suspensión/reanudación. |

El tamaño del ejecutable se medirá por separado del paquete instalado, las dependencias y los recursos. No se mantiene como obligación el dato de ~208 KB del prototipo mientras se incorpora contenido completo. Si un presupuesto resulta inviable, ajustar calidad/distancia o implementar una optimización sustentada en perfiles antes de redefinir el alcance de paridad.

## Cómo convertir cada fase en trabajo ejecutable

1. Abrir su especificación y comprobar dependencias de entrada con las evidencias de la fase anterior.
2. Separar sus pasos en tareas pequeñas con responsable, interfaces afectadas, fixtures, criterio de aceptación y migración si aplica.
3. Implementar primero un recorrido vertical real: entrada → validación → simulación → guardado → render/UI → red cuando corresponda.
4. Incorporar los casos límite antes de ampliar el catálogo: concurrencia, inventario lleno, bordes de chunk, desconexión y datos corruptos.
5. Ejecutar pruebas unitarias de invariantes, integración sin ventana, prueba jugable y perfil del escenario afectado. Las pruebas no deben limitarse a reproducir la implementación.
6. Entregar demo reproducible, datos de prueba, resultados y cambios de documentación. Cerrar solo después de pasar la puerta de salida específica.
7. Reestimar el siguiente bloque usando esfuerzo observado y riesgos resueltos. Los lotes de catálogo que excedan una fase se registran por ID para F15, sin marcar su paridad completa.

Para estimar calendario: medir capacidad real después de F00/F01, descomponer cada fase G/XL, estimar rangos por tarea y reservar integración/pruebas. No dividir mecánicamente un total entre desarrolladores: núcleo, formato de guardado y contratos de red forman dependencias críticas. El cierre de catálogo probablemente requiera muchas entregas; su alcance no cabe en una fecha confiable sin inventariarlo.

## Evidencia y definición de terminado

En la implementación futura, crear un expediente por fase en `docs/referencia/evidencias/FXX/` con:

- Build/revisión, versiones de herramientas, semilla, configuración y hardware.
- Casos ejecutados, esperado/obtenido, logs, métricas originales y capturas cuando aporten evidencia.
- Fixtures o instrucciones para reproducir y recuperar partidas.
- Lista de requisitos cerrados y diferencias abiertas con impacto, responsable y siguiente paso.

Un checkbox marcado sin evidencia no cierra una fase. Son bloqueadores la pérdida de mundos, duplicación de recursos, imposibilidad de completar progresión obligatoria, caída reproducible y desincronización permanente. Los defectos menores pueden registrarse siempre que no incumplan un criterio obligatorio.

La cobertura se calcula por separado para sistemas, contenido, comportamientos y plataformas. El denominador es el catálogo congelado en F00, no el conjunto de funciones ya implementadas. Si se excluye una mecánica del juego de referencia por decisión de diseño, debe figurar como diferencia: se puede lanzar un buen juego con esa diferencia, pero no afirmar igualdad completa.

## Riesgos principales y respuesta

| Riesgo | Respuesta planificada |
| --- | --- |
| Referencia crece durante el desarrollo | Congelar 1.21; backlog independiente para versiones posteriores y reevaluación explícita. |
| Reescritura masiva del prototipo | Extraer sistemas incrementalmente en F01 manteniendo construcción jugable. |
| Corrupción o duplicación | Persistencia transaccional F02, inventario F04, concurrencia F09 y pruebas cruzadas F15. |
| Coste móvil al añadir profundidad | Secciones dispersas, presupuestos desde F00 y perfiles por fase; medir temperaturas y sesiones largas. |
| Cinabrio aparentemente similar pero incompatible | Fixtures de secuencias por tick y diferencias documentadas en F11/F15. |
| Catálogo enorme escondido bajo “pulido” | Enumerarlo en F00, repartir responsables por familia y cerrar contenido en lotes durante todas las fases. |
| Android/iOS declarados sin pruebas | Prototipos tempranos; publicación de soporte únicamente tras instalación y recorrido real en F14. |
| Documentación sobrestima funciones | Actualizar README y catálogo cuando la evidencia confirma cada entrega. |

## Primeras acciones recomendadas

Comenzar por F00: confirmar el objetivo Java 1.21 en el registro de decisiones, enumerar catálogo, reproducir compilación y establecer hardware de referencia. Después resolver F01 —especialmente la separación de simulación y validación de red— y F02. Estas acciones habilitan trabajo posterior sin seguir acumulando funciones sobre un estado que aún se pierde al cerrar el juego.

Este conjunto de documentos entrega la comparación y el plan; las pruebas de aceptación y módulos descritos son trabajo de implementación futuro. La revisión que originó el plan inspeccionó código y consultó fuentes, pero no ejecutó el juego ni validó rendimiento o multijugador en hardware real.
