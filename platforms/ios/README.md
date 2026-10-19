# OpenCraft — Soporte y Compilación en iOS

Este directorio contiene la configuración y metadatos para la integración en **iOS (iPhone e iPad)** con **SDL3** y Metal/OpenGL ES.

## Requisitos de Entorno

- **macOS:** con Xcode 15+ y Command Line Tools.
- **SDK Objetivo:** iOS 15.0 o superior.
- **Arquitectura:** `arm64`.
- **Backend Gráfico:** Metal / OpenGLES vía MoltenVK/SDL3.

## Procedimiento de Generación de Proyecto Xcode con CMake

```bash
# 1. Crear directorio de compilación para iOS
mkdir -p build-ios && cd build-ios

# 2. Generar proyecto Xcode con toolchain iOS
cmake .. -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DOPENCRAFT_BUILD_TESTS=OFF

# 3. Compilar proyecto o abrir en Xcode
xcodebuild -scheme OpenCraft -configuration Release -destination 'generic/platform=iOS' build
```

## Características Verificadas en Fase 14

- Área segura (`safe-area-insets` para isla dinámica y notch de iPhone).
- Ciclo de vida iOS: gestión de suspensión al cambiar de app o recibir llamada.
- Solicitud explícita de permiso `NSLocalNetworkUsageDescription` para multijugador LAN sin alertas bloqueantes.
