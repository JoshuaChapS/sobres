#include "recurrencia.h"

namespace sobres {
namespace {

// Tope de seguridad para las búsquedas por conteo: 400 años de periodos
// semanales. Ninguna meta ni recurrente razonable llega ahí, y evita que una
// fecha absurda cuelgue la aplicación.
constexpr long long kTopePeriodos = 21000;

}  // namespace

Fecha avanzarPeriodos(const Fecha& desde, Periodicidad p, long long k) {
  switch (p) {
    case Periodicidad::Semanal:
      return sumarDias(desde, 7 * k);
    case Periodicidad::Mensual:
      return sumarMeses(desde, static_cast<int>(k));
    case Periodicidad::Quincenal: {
      // División que redondea hacia abajo también con negativos, para que
      // avanzar -1 quincena sea el reflejo exacto de avanzar 1.
      long long meses = k >= 0 ? k / 2 : -((-k + 1) / 2);
      long long resto = k - meses * 2;  // 0 o 1
      Fecha base = sumarMeses(desde, static_cast<int>(meses));
      return resto == 0 ? base : sumarDias(base, 15);
    }
  }
  return desde;
}

long long periodosEntre(const Fecha& desde, const Fecha& hasta,
                        Periodicidad p) {
  if (hasta < desde) return 0;
  // Para semanal el conteo es directo; para los otros dos el calendario no es
  // uniforme, así que se avanza hasta pasarse.
  if (p == Periodicidad::Semanal) {
    return diasEntre(desde, hasta) / 7;
  }
  long long k = 0;
  while (k < kTopePeriodos) {
    if (avanzarPeriodos(desde, p, k + 1) > hasta) break;
    ++k;
  }
  return k;
}

long long materializacionesPendientes(const Recurrente& r, const Fecha& hoy) {
  if (!r.activo) return 0;
  if (hoy < r.proximaFecha) return 0;
  // La fecha próxima ya venció, así que cuenta como una; a partir de ahí se
  // suman los periodos completos que hayan pasado.
  return periodosEntre(r.proximaFecha, hoy, r.periodicidad) + 1;
}

}  // namespace sobres
