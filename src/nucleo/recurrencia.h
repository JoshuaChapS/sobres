// Sobres — recurrencia.h
// Cómo avanza el calendario de un periodo al siguiente. Lo usan tanto los
// movimientos recurrentes como la proyección de metas, así que vive aparte.
#pragma once

#include "modelos.h"

namespace sobres {

// Avanza k periodos desde una fecha.
//   Semanal:   7 días por periodo.
//   Quincenal: dos periodos por mes. Un periodo es medio mes: avanzar 2 cae en
//              el mismo día del mes siguiente, avanzar 1 cae 15 días después.
//   Mensual:   un mes, recortando el día al último del mes cuando no existe.
// k puede ser negativo.
Fecha avanzarPeriodos(const Fecha& desde, Periodicidad p, long long k);

inline Fecha siguienteFecha(const Fecha& desde, Periodicidad p) {
  return avanzarPeriodos(desde, p, 1);
}

// Cuántos periodos completos caben entre dos fechas, es decir el mayor k tal
// que avanzarPeriodos(desde, p, k) <= hasta. Devuelve 0 si hasta es anterior
// a desde.
long long periodosEntre(const Fecha& desde, const Fecha& hasta, Periodicidad p);

// Cuántas veces habría que materializar un recurrente para ponerlo al día,
// contando la fecha de hoy como vencida.
long long materializacionesPendientes(const Recurrente& r, const Fecha& hoy);

}  // namespace sobres
