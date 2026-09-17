// Sobres — presupuesto.cpp
// Implementación de presupuesto.h. Ver ese archivo para el contrato.
#include "presupuesto.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace sobres {

Centavos disponible(const Presupuesto& p) { return p.asignado - p.gastado; }

int porcentajeUsado(const Presupuesto& p) {
  // Sin presupuesto asignado no hay proporción que calcular: o no se gastó
  // nada (0%), o se gastó algo sobre una base de cero, que el contrato define
  // como 100%.
  if (p.asignado == 0) return p.gastado == 0 ? 0 : 100;

  // División entera con redondeo al más cercano, medios alejándose del cero.
  // Se normaliza el signo del divisor para que el resto tenga el signo del
  // numerador y la comparación 2*|resto| >= |divisor| sea válida.
  Centavos numerador = p.gastado * 100;
  Centavos divisor = p.asignado;
  if (divisor < 0) {
    numerador = -numerador;
    divisor = -divisor;
  }

  Centavos cociente = numerador / divisor;  // trunca hacia cero
  Centavos resto = numerador % divisor;     // signo del numerador
  Centavos restoAbs = resto < 0 ? -resto : resto;

  if (restoAbs * 2 >= divisor) cociente += (numerador < 0 ? -1 : 1);

  return static_cast<int>(cociente);
}

EstadoPresupuesto evaluar(const Presupuesto& p) {
  const int usado = porcentajeUsado(p);
  if (usado < 80) return EstadoPresupuesto::Sano;
  if (usado <= 100) return EstadoPresupuesto::Advertencia;
  return EstadoPresupuesto::Excedido;
}

namespace {

const char* palabraDe(EstadoPresupuesto estado) {
  switch (estado) {
    case EstadoPresupuesto::Sano:
      return "sano";
    case EstadoPresupuesto::Advertencia:
      return "advertencia";
    case EstadoPresupuesto::Excedido:
      return "excedido";
  }
  return "";  // inalcanzable; calla el warning de "control reaches end"
}

}  // namespace

std::string resumen(const Presupuesto& p) {
  // El formateo de montos ya vive en dinero.h (formatearPesos); reusarlo
  // mantiene un solo lugar donde vive el formato del dinero de la app.
  return "Gastado " + formatearPesos(p.gastado) + " de " +
         formatearPesos(p.asignado) + " (" +
         std::to_string(porcentajeUsado(p)) + "%) - " +
         palabraDe(evaluar(p));
}

std::vector<std::size_t> masPresionados(const std::vector<Presupuesto>& ps,
                                        std::size_t n) {
  std::vector<std::size_t> indices(ps.size());
  for (std::size_t i = 0; i < ps.size(); ++i) indices[i] = i;

  // Orden: porcentaje descendente; a igual porcentaje, gastado descendente.
  // stable_sort conserva el orden original cuando ambos empatan, así que el
  // resultado es determinista.
  std::stable_sort(indices.begin(), indices.end(),
                   [&ps](std::size_t a, std::size_t b) {
                     const int usoA = porcentajeUsado(ps[a]);
                     const int usoB = porcentajeUsado(ps[b]);
                     if (usoA != usoB) return usoA > usoB;
                     return ps[a].gastado > ps[b].gastado;
                   });

  if (n < indices.size()) indices.resize(n);
  return indices;
}

}  // namespace sobres
