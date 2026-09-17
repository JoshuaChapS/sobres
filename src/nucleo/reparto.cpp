#include "reparto.h"

#include <algorithm>
#include <numeric>

namespace sobres {
namespace {

constexpr std::int64_t kPuntosBaseTotales = 10000;  // 100.00%

}  // namespace

std::int64_t sumaDePuntosBase(const PlantillaReparto& plantilla) {
  std::int64_t suma = 0;
  for (const LineaReparto& l : plantilla.lineas) suma += l.valor;
  return suma;
}

std::string formatearPuntosBase(std::int64_t puntosBase) {
  const bool negativo = puntosBase < 0;
  std::int64_t absoluto = negativo ? -puntosBase : puntosBase;
  const std::int64_t enteros = absoluto / 100;
  const std::int64_t decimales = absoluto % 100;
  std::string texto = (negativo ? "-" : "") + std::to_string(enteros);
  if (decimales != 0) {
    texto += '.';
    if (decimales % 10 == 0) {
      texto += std::to_string(decimales / 10);
    } else {
      if (decimales < 10) texto += '0';
      texto += std::to_string(decimales);
    }
  }
  return texto + "%";
}

ResultadoReparto calcularReparto(const PlantillaReparto& plantilla,
                                 Centavos montoTotal) {
  ResultadoReparto r;
  if (plantilla.lineas.empty()) {
    r.problema = "La plantilla no tiene ninguna línea.";
    return r;
  }
  for (const LineaReparto& l : plantilla.lineas) {
    if (l.sobre == kSinId) {
      r.problema = "Hay una línea sin sobre asignado.";
      return r;
    }
    if (l.valor < 0) {
      r.problema = "Ninguna línea puede tener un valor negativo.";
      return r;
    }
  }

  if (plantilla.modo == ModoReparto::MontosFijos) {
    for (const LineaReparto& l : plantilla.lineas) {
      r.asignaciones.push_back({l.sobre, static_cast<Centavos>(l.valor)});
      r.total += static_cast<Centavos>(l.valor);
    }
    if (r.total <= 0) {
      r.asignaciones.clear();
      r.total = 0;
      r.problema = "La suma de los montos de la plantilla es cero.";
      return r;
    }
    r.valido = true;
    return r;
  }

  // Modo por porcentajes.
  const std::int64_t suma = sumaDePuntosBase(plantilla);
  if (suma != kPuntosBaseTotales) {
    r.problema = "Los porcentajes suman " + formatearPuntosBase(suma) +
                 " y deben sumar exactamente 100%.";
    return r;
  }
  if (montoTotal <= 0) {
    r.problema = "El monto a repartir tiene que ser mayor que cero.";
    return r;
  }

  const std::size_t cuantas = plantilla.lineas.size();
  std::vector<Centavos> parte(cuantas, 0);
  std::vector<std::int64_t> residuo(cuantas, 0);
  Centavos asignado = 0;

  for (std::size_t k = 0; k < cuantas; ++k) {
    const std::int64_t producto = montoTotal * plantilla.lineas[k].valor;
    parte[k] = producto / kPuntosBaseTotales;
    residuo[k] = producto % kPuntosBaseTotales;
    asignado += parte[k];
  }

  // Centavos que sobran por el redondeo hacia abajo. Son menos que el número
  // de líneas, siempre.
  Centavos sobrantes = montoTotal - asignado;

  std::vector<std::size_t> orden(cuantas);
  std::iota(orden.begin(), orden.end(), 0);
  std::stable_sort(orden.begin(), orden.end(),
                   [&](std::size_t a, std::size_t b) {
                     const bool restoA = plantilla.lineas[a].recibeResto;
                     const bool restoB = plantilla.lineas[b].recibeResto;
                     if (restoA != restoB) return restoA;       // marcadas primero
                     if (residuo[a] != residuo[b]) return residuo[a] > residuo[b];
                     return a < b;  // empate: gana la línea de más arriba
                   });

  for (std::size_t k = 0; k < orden.size() && sobrantes > 0; ++k) {
    ++parte[orden[k]];
    --sobrantes;
  }

  for (std::size_t k = 0; k < cuantas; ++k) {
    r.asignaciones.push_back({plantilla.lineas[k].sobre, parte[k]});
  }
  r.total = montoTotal;
  r.valido = true;
  return r;
}

std::vector<Movimiento> generarMovimientosDeReparto(
    const PlantillaReparto& plantilla, const ResultadoReparto& resultado,
    const Fecha& fecha) {
  std::vector<Movimiento> movimientos;
  if (!resultado.valido) return movimientos;

  for (const AsignacionReparto& a : resultado.asignaciones) {
    if (a.monto == 0) continue;  // no se guardan movimientos en cero
    Movimiento m;
    m.fecha = fecha;
    m.monto = a.monto;
    m.categoria = plantilla.categoria;
    m.nota = "Reparto: " + plantilla.nombre;
    m.origenAutomatico = "reparto:" + plantilla.nombre;
    if (plantilla.sobreFuente == kSinId) {
      m.tipo = TipoMovimiento::Entrada;
      m.sobreOrigen = a.sobre;
    } else {
      m.tipo = TipoMovimiento::Traspaso;
      m.sobreOrigen = plantilla.sobreFuente;
      m.sobreDestino = a.sobre;
    }
    movimientos.push_back(m);
  }
  return movimientos;
}

}  // namespace sobres
