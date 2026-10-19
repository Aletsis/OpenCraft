# Fase 00 — Referencia, auditoría y calidad

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: propuesta, no implementada por esta documentación. Dependencias de entrada: **Ninguna**. Brechas: **B21, B22, B23**.

## Qué lograr, por qué y para qué

Convertir “igualar el juego de referencia” en un conjunto finito de requisitos y establecer una base reproducible. Evita desarrollar contra una referencia cambiante y confundir declaraciones del README con capacidades comprobadas.

## Alcance y límites

Inventario de capacidades, catálogo de referencia Java 1.21, decisiones Java/Bedrock, compilación Linux limpia, pruebas del prototipo y perfiles de hardware. Todavía no implementa contenido del juego de referencia. Android/iOS requieren una prueba temprana de viabilidad de compilación y contexto gráfico, aunque su acabado llega en F14.

## Pasos de implementación

1. Crear `docs/referencia/catalogo.csv` durante la implementación: ID, familia, elemento/regla, versión, fuente, comportamiento esperado, equivalente OpenCraft, fase, estado y prueba. Enumerar bloques/estados, ítems, recetas, criaturas, biomas, estructuras, efectos, encantamientos, comandos y avances. Una categoría genérica no sustituye sus filas individuales.

2. Fijar Java 1.21 y anotar discrepancias con Bedrock, especialmente combate, cinabrio, modos y controles. Congelar fuentes/revisión y fecha por requisito; los casos no confirmados quedan como pendientes, nunca como cumplidos.

3. Auditar los hallazgos de COMPARACION.md y abrir tareas con escenario reproducible, prioridad y responsable. Medir el prototipo en vez de conservar cifras sin contexto.

4. Separar una compilación de verificación de `build/`; registrar versiones de compilador, SDL3 y GLES. Preparar CI, CTest y ejecutables de prueba de lógica sin ventana. Añadir fixtures para coordenadas negativas, raycast y colocación.

5. Escoger un equipo Linux x86_64 y uno ARM64 móvil, registrar CPU/GPU/RAM/SO/controlador, resolución y energía. Probar una ventana SDL/GLES y empaquetado mínimo Android/iOS con herramientas disponibles; documentar bloqueos externos concretos.

6. Crear mundos de prueba y guiones para construcción, exploración, guardado y LAN. Registrar p50/p95/p99 de frame y tick, memoria, chunks y tiempos de carga; actualizar README con estados comprobados.

## Entregables y estado al finalizar

Catálogo versionado completo en cuanto a inventario de entradas, backlog priorizado, registro de decisiones, CI y protocolo de medición. Informe de plataforma con resultado real o bloqueo explícito; informe de línea base sin afirmar paridad.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Repositorio limpio y dependencias documentadas | Configurar y compilar en directorio nuevo siguiendo únicamente las instrucciones | La compilación es reproducible; versiones y comando quedan registrados. |
| Pruebas de lógica | Ejecutar CTest con al menos un caso válido y uno de frontera para coordenadas y raycast | Los resultados detectan una regresión introducida deliberadamente y vuelven a pasar al revertirla. |
| Catálogo de referencia | Reconciliar el listado por familias con las fuentes fijadas y revisar filas sin fase/prueba | Cero filas sin identificación y propietario de fase; las reglas todavía no verificadas están señaladas. |
| Misma escena y equipo | Ejecutar tres recorridos de diez minutos tras calentamiento | Informe con configuración, distribuciones y muestras originales; ninguna cifra de rendimiento sin contexto. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: enumerar solo ejemplos y llamar a eso catálogo completo. Mitigar con reconciliación por familias y revisión independiente del inventario. Si una plataforma no puede probarse, no etiquetarla como soportada; conservar el requisito abierto para F14.

## Puerta de salida

- [ ] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [ ] La regresión de fases anteriores afectadas sigue pasando.
- [ ] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [ ] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [ ] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
