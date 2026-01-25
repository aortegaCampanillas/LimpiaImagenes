# Limpia Imagenes (C++)

Version en C++ en una carpeta independiente. Incluye una app GUI (Qt) y
una version de linea de comandos que aplica el mismo algoritmo.

## Requisitos
- CMake 3.16+
- OpenCV (incluyendo headers y librerias)

## Compilar
Desde la carpeta `cpp_app`:

```bash
cmake -S . -B build
cmake --build build
```

## Ejecutar (GUI)
```bash
./build/limpia_imagenes_gui
```

## Ejecutar (CLI)
```bash
./build/limpia_imagenes_cpp --input ruta/imagen.jpg --output salida.png
```

Opcionalmente:
```bash
./build/limpia_imagenes_cpp --input imagen.jpg --output salida.png \
  --config ../config.json
```
