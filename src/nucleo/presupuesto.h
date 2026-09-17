// Sobres — presupuesto.h
// Evaluar un sobre contra su presupuesto: cuánto queda, qué tan presionado va,
// y un resumen legible para la interfaz.
//
// Todo el dinero entra y sale en Centavos (ver dinero.h). Los porcentajes van
// como enteros redondeados al más cercano; no se usa punto flotante fuera del
// cálculo intermedio.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dinero.h"

namespace sobres {

enum class EstadoPresupuesto { Sano, Advertencia, Excedido };

struct Presupuesto {
  Centavos asignado = 0;
  Centavos gastado = 0;
};

// Lo que queda por gastar. Es negativo si el sobre se excedió.
Centavos disponible(const Presupuesto& p);

// Porcentaje del asignado que ya se gastó, redondeado al entero más cercano
// (los medios se alejan del cero). Puede pasar de 100.
//
// Caso especial, presupuesto en cero: si no se gastó nada devuelve 0; si se
// gastó cualquier cantidad devuelve 100.
int porcentajeUsado(const Presupuesto& p);

// Estado según el porcentaje usado:
//     menos de 80        -> Sano
//     de 80 a 100        -> Advertencia
//     más de 100         -> Excedido
EstadoPresupuesto evaluar(const Presupuesto& p);

// Resumen de una línea para la capa de presentación. Formato exacto:
//
//     Gastado $800.00 de $1,000.00 (80%) - advertencia
//
// La palabra final es "sano", "advertencia" o "excedido" según evaluar().
// Los montos se formatean con el formateador que ya existe en el proyecto.
std::string resumen(const Presupuesto& p);

// Los índices de los n presupuestos con mayor porcentaje usado, ordenados de
// mayor a menor. Si dos empatan en porcentaje, va primero el que gastó más.
// Si se piden más de los que hay, devuelve todos.
std::vector<std::size_t> masPresionados(const std::vector<Presupuesto>& ps,
                                        std::size_t n);

}  // namespace sobres
