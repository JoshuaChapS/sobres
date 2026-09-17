cmake_minimum_required(VERSION 3.16)
project(Sobres VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /utf-8)
else()
  add_compile_options(-Wall -Wextra)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

# --- Núcleo: la lógica financiera, sin ninguna dependencia de Qt -------------
# Se mantiene separado a propósito: así las pruebas de la matemática de metas,
# el reparto de centavos y el reporte corren con solo un compilador de C++.
add_library(sobres_nucleo STATIC
  src/nucleo/dinero.cpp
  src/nucleo/fecha.cpp
  src/nucleo/modelos.cpp
  src/nucleo/saldos.cpp
  src/nucleo/recurrencia.cpp
  src/nucleo/meta_proyeccion.cpp
  src/nucleo/reparto.cpp
  src/nucleo/reporte.cpp
  src/nucleo/validacion.cpp
  src/nucleo/xlsx.cpp
  src/nucleo/exportacion.cpp
  src/nucleo/presupuesto.cpp
)
target_include_directories(sobres_nucleo PUBLIC src)

# --- Datos: SQLite a través de QtSql ----------------------------------------
add_library(sobres_datos STATIC
  src/datos/repositorio.cpp
)
target_link_libraries(sobres_datos PUBLIC sobres_nucleo Qt6::Core Qt6::Sql)

# --- Aplicación de escritorio -----------------------------------------------
add_executable(sobres
  src/main.cpp
  src/ui/estado.cpp
  src/ui/comunes.cpp
  src/ui/dialogos.cpp
  src/ui/pagina_inicio.cpp
  src/ui/pagina_movimientos.cpp
  src/ui/pagina_metas.cpp
  src/ui/pagina_automatizaciones.cpp
  src/ui/pagina_reporte.cpp
  src/ui/pagina_ajustes.cpp
  src/ui/ventana_principal.cpp
)
target_link_libraries(sobres PRIVATE sobres_datos Qt6::Widgets Qt6::Gui)

if(WIN32)
  set_target_properties(sobres PROPERTIES WIN32_EXECUTABLE ON)
endif()

# --- Pruebas ----------------------------------------------------------------
enable_testing()

add_library(marco_pruebas STATIC pruebas/marco.cpp)
target_include_directories(marco_pruebas PUBLIC pruebas)

add_executable(pruebas_nucleo
  pruebas/principal_nucleo.cpp
  pruebas/prueba_dinero.cpp
  pruebas/prueba_fecha.cpp
  pruebas/prueba_saldos.cpp
  pruebas/prueba_meta.cpp
  pruebas/prueba_reparto.cpp
  pruebas/prueba_recurrencia.cpp
  pruebas/prueba_reporte.cpp
  pruebas/prueba_validacion.cpp
  pruebas/prueba_xlsx.cpp
  pruebas/prueba_exportacion.cpp
  pruebas/prueba_presupuesto.cpp
)
target_link_libraries(pruebas_nucleo PRIVATE sobres_nucleo marco_pruebas)
add_test(NAME nucleo COMMAND pruebas_nucleo)

add_executable(pruebas_datos
  pruebas/principal_datos.cpp
  pruebas/prueba_repositorio.cpp
)
target_link_libraries(pruebas_datos PRIVATE sobres_datos marco_pruebas Qt6::Core)
add_test(NAME datos COMMAND pruebas_datos)
