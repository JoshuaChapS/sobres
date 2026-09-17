// Sobres — saldos.h
// El cálculo del que depende toda la pantalla principal: cuánto hay en cada
// sobre, cuánto hay en la cuenta y cuál es el patrimonio real.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "modelos.h"

namespace sobres {

// Saldo de cada sobre, calculado desde cero recorriendo los movimientos. No se
// guarda un saldo acumulado en la base de datos a propósito: así el saldo nunca
// puede quedar desincronizado de los movimientos que lo explican.
// Los sobres archivados también aparecen: si les quedó dinero, ese dinero
// sigue estando en la cuenta.
std::map<Id, Centavos> calcularSaldos(const std::vector<Sobre>& sobres,
                                      const std::vector<Movimiento>& movimientos);

struct ResumenPatrimonio {
  // Lo que el banco dice que hay: la suma de todos los sobres, incluidos los
  // de terceros.
  Centavos enLaCuenta = 0;
  // Dinero que está de paso y no es del usuario.
  Centavos dineroAjeno = 0;
  // Sobres propios y de meta, que sí son del usuario.
  Centavos dineroPropio = 0;
  // Suma de las deudas registradas.
  Centavos pasivos = 0;
  // El número central de la aplicación.
  Centavos patrimonioReal = 0;
};

ResumenPatrimonio calcularPatrimonio(const std::vector<Sobre>& sobres,
                                     const std::map<Id, Centavos>& saldos,
                                     const std::vector<Pasivo>& pasivos);

// La línea que va debajo de las dos cifras y explica en palabras por qué no
// son iguales.
std::string explicarDiferencia(const ResumenPatrimonio& resumen);

}  // namespace sobres
