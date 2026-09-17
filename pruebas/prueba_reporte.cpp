#include "../src/nucleo/reporte.h"

#include "marco.h"

using namespace sobres;

namespace {

std::vector<Sobre> sobresDePrueba() {
  Sobre diario;
  diario.id = 1;
  diario.nombre = "Gasto diario";
  Sobre casa;
  casa.id = 2;
  casa.nombre = "Casa";
  return {diario, casa};
}

std::vector<Categoria> categoriasDePrueba() {
  Categoria comida;
  comida.id = 10;
  comida.nombre = "Comida";
  Categoria transporte;
  transporte.id = 11;
  transporte.nombre = "Transporte";
  return {comida, transporte};
}

Movimiento gasto(const Fecha& f, Id sobre, Id categoria, Centavos monto) {
  Movimiento m;
  m.fecha = f;
  m.tipo = TipoMovimiento::Gasto;
  m.sobreOrigen = sobre;
  m.categoria = categoria;
  m.monto = monto;
  return m;
}

}  // namespace

PRUEBA(reporte_agrupa_gasto_por_categoria_y_compara_con_el_mes_anterior) {
  std::vector<Movimiento> movimientos = {
      gasto(Fecha{2026, 9, 3}, 1, 10, 150000),   // comida, septiembre
      gasto(Fecha{2026, 9, 20}, 1, 10, 50000),   // comida, septiembre
      gasto(Fecha{2026, 9, 10}, 2, 11, 30000),   // transporte, septiembre
      gasto(Fecha{2026, 8, 15}, 1, 10, 250000),  // comida, agosto
      gasto(Fecha{2026, 7, 15}, 1, 10, 999999),  // julio, fuera del reporte
  };

  const ReporteMensual r = generarReporteMensual(
      2026, 9, sobresDePrueba(), categoriasDePrueba(), movimientos);

  VERIFICAR_IGUAL(r.titulo, std::string("septiembre 2026"));
  VERIFICAR_IGUAL(r.gastoTotal, static_cast<Centavos>(230000));
  VERIFICAR_IGUAL(r.gastoTotalAnterior, static_cast<Centavos>(250000));
  VERIFICAR_IGUAL(r.diferenciaGasto, static_cast<Centavos>(-20000));

  VERIFICAR_IGUAL(r.porCategoria.size(), static_cast<std::size_t>(2));
  VERIFICAR_IGUAL(r.porCategoria[0].nombre, std::string("Comida"));
  VERIFICAR_IGUAL(r.porCategoria[0].mesActual, static_cast<Centavos>(200000));
  VERIFICAR_IGUAL(r.porCategoria[0].mesAnterior, static_cast<Centavos>(250000));
  VERIFICAR_IGUAL(r.porCategoria[0].diferencia, static_cast<Centavos>(-50000));
  VERIFICAR(r.porCategoria[0].hayComparacion);
  VERIFICAR_CERCA(r.porCategoria[0].variacion, -0.2, 1e-12);

  // Transporte no tuvo gasto el mes pasado, así que no hay porcentaje.
  VERIFICAR_IGUAL(r.porCategoria[1].nombre, std::string("Transporte"));
  VERIFICAR(!r.porCategoria[1].hayComparacion);
}

PRUEBA(reporte_agrupa_gasto_por_sobre) {
  std::vector<Movimiento> movimientos = {
      gasto(Fecha{2026, 9, 3}, 1, 10, 150000),
      gasto(Fecha{2026, 9, 10}, 2, 11, 300000),
  };
  const ReporteMensual r = generarReporteMensual(
      2026, 9, sobresDePrueba(), categoriasDePrueba(), movimientos);

  VERIFICAR_IGUAL(r.porSobre.size(), static_cast<std::size_t>(2));
  VERIFICAR_IGUAL(r.porSobre[0].nombre, std::string("Casa"));
  VERIFICAR_IGUAL(r.porSobre[0].mesActual, static_cast<Centavos>(300000));
  VERIFICAR_IGUAL(r.porSobre[1].nombre, std::string("Gasto diario"));
}

PRUEBA(reporte_separa_los_gastos_sin_categoria) {
  std::vector<Movimiento> movimientos = {
      gasto(Fecha{2026, 9, 3}, 1, kSinId, 40000),
      gasto(Fecha{2026, 9, 4}, 1, 10, 10000),
  };
  const ReporteMensual r = generarReporteMensual(
      2026, 9, sobresDePrueba(), categoriasDePrueba(), movimientos);

  VERIFICAR_IGUAL(r.porCategoria.size(), static_cast<std::size_t>(2));
  VERIFICAR_IGUAL(r.porCategoria[0].nombre, std::string("Sin categoría"));
  VERIFICAR_IGUAL(r.porCategoria[0].mesActual, static_cast<Centavos>(40000));
}

PRUEBA(reporte_no_cuenta_traspasos_como_gasto) {
  Movimiento traspaso;
  traspaso.fecha = Fecha{2026, 9, 5};
  traspaso.tipo = TipoMovimiento::Traspaso;
  traspaso.sobreOrigen = 1;
  traspaso.sobreDestino = 2;
  traspaso.monto = 500000;

  Movimiento entrada;
  entrada.fecha = Fecha{2026, 9, 1};
  entrada.tipo = TipoMovimiento::Entrada;
  entrada.sobreOrigen = 1;
  entrada.monto = 1200000;

  const ReporteMensual r = generarReporteMensual(
      2026, 9, sobresDePrueba(), categoriasDePrueba(), {traspaso, entrada});

  VERIFICAR_IGUAL(r.gastoTotal, static_cast<Centavos>(0));
  VERIFICAR_IGUAL(r.entradasTotal, static_cast<Centavos>(1200000));
  VERIFICAR(r.porCategoria.empty());
  VERIFICAR(r.porSobre.empty());
}

PRUEBA(reporte_cruza_bien_el_cambio_de_anio) {
  std::vector<Movimiento> movimientos = {
      gasto(Fecha{2026, 1, 5}, 1, 10, 100000),
      gasto(Fecha{2025, 12, 20}, 1, 10, 80000),
  };
  const ReporteMensual r = generarReporteMensual(
      2026, 1, sobresDePrueba(), categoriasDePrueba(), movimientos);

  VERIFICAR_IGUAL(r.gastoTotal, static_cast<Centavos>(100000));
  VERIFICAR_IGUAL(r.gastoTotalAnterior, static_cast<Centavos>(80000));
  VERIFICAR(resumirComparacion(r).find("diciembre 2025") != std::string::npos);
}

PRUEBA(reporte_resume_la_comparacion_en_palabras) {
  std::vector<Movimiento> movimientos = {
      gasto(Fecha{2026, 9, 3}, 1, 10, 120000),
      gasto(Fecha{2026, 8, 3}, 1, 10, 100000),
  };
  const ReporteMensual r = generarReporteMensual(
      2026, 9, sobresDePrueba(), categoriasDePrueba(), movimientos);
  const std::string texto = resumirComparacion(r);
  VERIFICAR(texto.find("$1,200.00") != std::string::npos);
  VERIFICAR(texto.find("más") != std::string::npos);
  VERIFICAR(texto.find("20.0%") != std::string::npos);

  const ReporteMensual vacio = generarReporteMensual(
      2026, 5, sobresDePrueba(), categoriasDePrueba(), movimientos);
  VERIFICAR(resumirComparacion(vacio).find("No hay gastos") !=
            std::string::npos);
}
