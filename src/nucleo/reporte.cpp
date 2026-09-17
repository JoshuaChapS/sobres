#include "reporte.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace sobres {
namespace {

bool esDelMes(const Fecha& f, int anio, int mes) {
  return f.anio == anio && f.mes == mes;
}

void completarComparacion(LineaReporte& linea) {
  linea.diferencia = linea.mesActual - linea.mesAnterior;
  if (linea.mesAnterior != 0) {
    linea.hayComparacion = true;
    linea.variacion = static_cast<double>(linea.diferencia) /
                      std::abs(static_cast<double>(linea.mesAnterior));
  }
}

void ordenarPorGasto(std::vector<LineaReporte>& lineas) {
  std::stable_sort(lineas.begin(), lineas.end(),
                   [](const LineaReporte& a, const LineaReporte& b) {
                     if (a.mesActual != b.mesActual) {
                       return a.mesActual > b.mesActual;
                     }
                     return a.mesAnterior > b.mesAnterior;
                   });
}

}  // namespace

ReporteMensual generarReporteMensual(
    int anio, int mes, const std::vector<Sobre>& sobres,
    const std::vector<Categoria>& categorias,
    const std::vector<Movimiento>& movimientos) {
  ReporteMensual r;
  r.anio = anio;
  r.mes = mes;
  r.titulo = nombreDeMes(mes) + " " + std::to_string(anio);

  const Fecha anterior = sumarMeses(Fecha{anio, mes, 1}, -1);

  std::map<Id, Centavos> gastoCategoriaActual, gastoCategoriaAnterior;
  std::map<Id, Centavos> gastoSobreActual, gastoSobreAnterior;

  for (const Movimiento& m : movimientos) {
    const bool actual = esDelMes(m.fecha, anio, mes);
    const bool previo = esDelMes(m.fecha, anterior.anio, anterior.mes);
    if (!actual && !previo) continue;

    if (m.tipo == TipoMovimiento::Entrada) {
      if (actual) r.entradasTotal += m.monto;
      else r.entradasTotalAnterior += m.monto;
      continue;
    }
    if (m.tipo != TipoMovimiento::Gasto) continue;  // los traspasos no gastan

    if (actual) {
      r.gastoTotal += m.monto;
      gastoCategoriaActual[m.categoria] += m.monto;
      gastoSobreActual[m.sobreOrigen] += m.monto;
    } else {
      r.gastoTotalAnterior += m.monto;
      gastoCategoriaAnterior[m.categoria] += m.monto;
      gastoSobreAnterior[m.sobreOrigen] += m.monto;
    }
  }
  r.diferenciaGasto = r.gastoTotal - r.gastoTotalAnterior;

  for (const Categoria& c : categorias) {
    const Centavos a = gastoCategoriaActual.count(c.id)
                           ? gastoCategoriaActual[c.id] : 0;
    const Centavos b = gastoCategoriaAnterior.count(c.id)
                           ? gastoCategoriaAnterior[c.id] : 0;
    if (a == 0 && b == 0) continue;  // no se listan categorías sin actividad
    LineaReporte linea;
    linea.id = c.id;
    linea.nombre = c.nombre;
    linea.color = c.color;
    linea.mesActual = a;
    linea.mesAnterior = b;
    completarComparacion(linea);
    r.porCategoria.push_back(linea);
  }
  // Los gastos que nadie clasificó también tienen que aparecer.
  const Centavos sinCatActual = gastoCategoriaActual.count(kSinId)
                                    ? gastoCategoriaActual[kSinId] : 0;
  const Centavos sinCatAnterior = gastoCategoriaAnterior.count(kSinId)
                                      ? gastoCategoriaAnterior[kSinId] : 0;
  if (sinCatActual != 0 || sinCatAnterior != 0) {
    LineaReporte linea;
    linea.id = kSinId;
    linea.nombre = "Sin categoría";
    linea.color = "#9ca3af";
    linea.mesActual = sinCatActual;
    linea.mesAnterior = sinCatAnterior;
    completarComparacion(linea);
    r.porCategoria.push_back(linea);
  }

  for (const Sobre& s : sobres) {
    const Centavos a = gastoSobreActual.count(s.id) ? gastoSobreActual[s.id] : 0;
    const Centavos b = gastoSobreAnterior.count(s.id)
                           ? gastoSobreAnterior[s.id] : 0;
    if (a == 0 && b == 0) continue;
    LineaReporte linea;
    linea.id = s.id;
    linea.nombre = s.nombre;
    linea.color = s.color;
    linea.mesActual = a;
    linea.mesAnterior = b;
    completarComparacion(linea);
    r.porSobre.push_back(linea);
  }

  ordenarPorGasto(r.porCategoria);
  ordenarPorGasto(r.porSobre);
  return r;
}

std::string resumirComparacion(const ReporteMensual& r) {
  const Fecha anterior = sumarMeses(Fecha{r.anio, r.mes, 1}, -1);
  const std::string nombreAnterior =
      nombreDeMes(anterior.mes) + " " + std::to_string(anterior.anio);

  if (r.gastoTotal == 0 && r.gastoTotalAnterior == 0) {
    return "No hay gastos registrados en " + r.titulo + " ni en " +
           nombreAnterior + ".";
  }
  if (r.gastoTotalAnterior == 0) {
    return "Gastaste " + formatearPesos(r.gastoTotal) + " en " + r.titulo +
           ". En " + nombreAnterior + " no hubo gastos registrados.";
  }
  if (r.diferenciaGasto == 0) {
    return "Gastaste " + formatearPesos(r.gastoTotal) + ", exactamente lo "
           "mismo que en " + nombreAnterior + ".";
  }

  const double porcentaje = 100.0 * static_cast<double>(r.diferenciaGasto) /
                            static_cast<double>(r.gastoTotalAnterior);
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.1f", std::abs(porcentaje));
  const std::string direccion = r.diferenciaGasto > 0 ? "más" : "menos";
  return "Gastaste " + formatearPesos(r.gastoTotal) + " en " + r.titulo +
         ", " + formatearPesos(std::abs(r.diferenciaGasto)) + " " + direccion +
         " que en " + nombreAnterior + " (" + buffer + "%).";
}

}  // namespace sobres
