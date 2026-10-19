# Política de Mantenimiento y Compatibilidad Hacia Adelante — OpenCraft

## 1. Alcance y Filosofía
OpenCraft se compromete a mantener la estabilidad arquitectónica, la paridad determinista con el juego de referencia y la compatibilidad estricta con versiones anteriores del motor.

---

## 2. Compatibilidad de Esquemas de Persistencia (World Format)

1. **Versionado de Esquemas:** Todo cambio en la estructura de almacenamiento de mundos (chunks, jugadores, entidades, bloques-entidad) debe incrementar `CURRENT_SCHEMA_VERSION` en `src/storage/WorldFormat.hpp`.
2. **Compatibilidad Hacia Adelante Inquebrantable:**
   - La función `WorldFormat::deserializeChunk` y el motor `WorldManager::migrateWorld` deben ser capaces de leer y actualizar cualquier mundo generado en esquemas anteriores (desde `SCHEMA_VERSION_1` hasta la versión actual).
   - Queda terminantemente prohibido eliminar código de lectura de esquemas antiguos sin una decisión arquitectónica explícita y justificada.
3. **Idempotencia de Migración:** Ejecutar una migración hacia un esquema en el que el mundo ya reside debe resultar en una operación segura sin alteraciones ni degradación de datos.
4. **Respaldo Obligatorio:** Toda migración de mundo en disco debe generar de manera obligatoria e inmediata una copia de seguridad en el directorio `backups/` antes de aplicar cambios destructivos al formato.

---

## 3. Adición de Contenido y Paridad con Futuras Versiones

1. **Regla de No Regresión:** La incorporación de nuevos bloques, ítems, criaturas o mecánicas (por ejemplo, futuras actualizaciones 1.22+) no debe romper las pruebas de regresión existentes en `tests/`.
2. **Conciliación en `catalogo.csv`:** Todo nuevo elemento debe registrarse previamente en `docs/referencia/catalogo.csv` especificando identificador canónico (`opencraft:*`), familia, estado de paridad y test de cobertura asociado.
3. **Determinismo:** Ningún sistema de simulación puede depender del reloj del sistema (`std::chrono::system_clock`) para la toma de decisiones lógicas en el mundo de juego; todo debe calcularse a partir del `SimulationClock` a 20 TPS.

---

## 4. Política de Lanzamientos y Versionado Semántico (SemVer)

- **Versiones Mayores (X.y.z):** Modificaciones profundas del pipeline de renderizado o arquitectura de red que requieran nueva especificación de protocolo cliente-servidor.
- **Versiones Menores (x.Y.z):** Incorporación de nuevo contenido de paridad con versiones del juego de referencia (bloques, dimensiones, biomas, mecánicas de juego).
- **Parches (x.y.Z):** Corrección de bugs lógicos, mejoras de rendimiento o optimizaciones de asignación de memoria sin alterar el formato de guardado ni el protocolo de red.
