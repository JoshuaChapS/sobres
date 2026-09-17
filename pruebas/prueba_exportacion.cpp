#include "../src/nucleo/exportacion.h"

#include "marco.h"

using namespace sobres;

namespace {

Sobre hacerSobre(Id id, const char* nombre, TipoSobre tipo = TipoSobre::Propio,
                 bool archivado = false) {
  Sobre s;
  s.id = id;
  s.nombre = nombre;
  s.tipo = tipo;
  s.archivado = archivado;
  return s;
}

Categoria hacerCategoria(Id id, const char* nombre) {
  Categoria c;
  c.id = id;
  c.nombre = nombre;
  return c;
}

Movimiento mov(Id id, const Fecha& f, TipoMovimiento tipo, Id origen,
               Id destino, Id categoria, Centavos monto,
               const char* nota = "") {
  Movimiento m;
  m.id = id;
  m.fecha = f;
  m.tipo = tipo;
  m.sobreOrigen = origen;
  m.sobreDestino = destino;
  m.categoria = categoria;
  m.monto = monto;
  m.nota = nota;
  return m;
}

struct Escenario {
  std::vector<Sobre> sobres = {hacerSobre(1, "Gasto diario"),
                               hacerSobre(2, "Casa")};
  std::vector<Categoria> categorias = {hacerCategoria(10, "Comida"),
                                       hacerCategoria(11, "Sueldo")};
  std::vector<Movimiento> movimientos = {
      // Agosto: queda como saldo inicial de septiembre.
      mov(1, Fecha{2026, 8, 15}, TipoMovimiento::Entrada, 1, kSinId, 11, 500000),
      mov(2, Fecha{2026, 8, 20}, TipoMovimiento::Gasto, 1, kSinId, 10, 100000),
      // Septiembre.
      mov(3, Fecha{2026, 9, 1}, TipoMovimiento::Entrada, 1, kSinId, 11, 1800000,
          "Sueldo"),
      mov(4, Fecha{2026, 9, 3}, TipoMovimiento::Gasto, 1, kSinId, 10, 47850,
          "Mercado"),
      mov(5, Fecha{2026, 9, 5}, TipoMovimiento::Traspaso, 1, 2, kSinId, 800000,
          "Para la renta"),
      mov(6, Fecha{2026, 9, 10}, TipoMovimiento::Gasto, 2, kSinId, 10, 20000),
  };
  std::vector<Pasivo> pasivos;
};

const ReporteDeSobre* buscar(const ReporteMensualPorSobre& r, Id sobre) {
  for (const ReporteDeSobre& s : r.sobres) {
    if (s.id == sobre) return &s;
  }
  return nullptr;
}

}  // namespace

PRUEBA(exportacion_calcula_el_saldo_inicial_del_mes) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);

  // Antes de septiembre el sobre diario tenía 5,000 menos 1,000.
  VERIFICAR_IGUAL(buscar(r, 1)->saldoInicial, static_cast<Centavos>(400000));
  VERIFICAR_IGUAL(buscar(r, 2)->saldoInicial, static_cast<Centavos>(0));
  VERIFICAR_IGUAL(r.enLaCuentaAlInicio, static_cast<Centavos>(400000));
}

PRUEBA(exportacion_cuadra_el_saldo_final_con_los_movimientos) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);

  const ReporteDeSobre* diario = buscar(r, 1);
  // 4,000 + 18,000 − 478.50 − 8,000 = 13,521.50
  VERIFICAR_IGUAL(diario->entradas, static_cast<Centavos>(1800000));
  VERIFICAR_IGUAL(diario->gastos, static_cast<Centavos>(47850));
  VERIFICAR_IGUAL(diario->traspasosEnviados, static_cast<Centavos>(800000));
  VERIFICAR_IGUAL(diario->traspasosRecibidos, static_cast<Centavos>(0));
  VERIFICAR_IGUAL(diario->saldoFinal, static_cast<Centavos>(1352150));

  const ReporteDeSobre* casa = buscar(r, 2);
  VERIFICAR_IGUAL(casa->traspasosRecibidos, static_cast<Centavos>(800000));
  VERIFICAR_IGUAL(casa->gastos, static_cast<Centavos>(20000));
  VERIFICAR_IGUAL(casa->saldoFinal, static_cast<Centavos>(780000));

  // El saldo final tiene que ser exactamente el inicial más todo lo del mes.
  for (const ReporteDeSobre& s : r.sobres) {
    const Centavos esperado = s.saldoInicial + s.entradas - s.gastos +
                              s.traspasosRecibidos - s.traspasosEnviados;
    VERIFICAR_IGUAL(s.saldoFinal, esperado);
  }
}

PRUEBA(exportacion_lleva_el_saldo_corriendo_renglon_por_renglon) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);
  const ReporteDeSobre* diario = buscar(r, 1);

  VERIFICAR_IGUAL(diario->movimientos.size(), static_cast<std::size_t>(3));
  // Van en orden cronológico y el saldo se va acumulando.
  VERIFICAR_IGUAL(diario->movimientos[0].saldoCorriente,
                  static_cast<Centavos>(2200000));
  VERIFICAR_IGUAL(diario->movimientos[1].saldoCorriente,
                  static_cast<Centavos>(2152150));
  VERIFICAR_IGUAL(diario->movimientos[2].saldoCorriente,
                  static_cast<Centavos>(1352150));
  // El último renglón coincide con el saldo final del sobre.
  VERIFICAR_IGUAL(diario->movimientos.back().saldoCorriente,
                  diario->saldoFinal);
}

PRUEBA(exportacion_firma_el_efecto_de_cada_movimiento) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);
  const ReporteDeSobre* diario = buscar(r, 1);

  VERIFICAR_IGUAL(diario->movimientos[0].efecto, static_cast<Centavos>(1800000));
  VERIFICAR_IGUAL(diario->movimientos[1].efecto, static_cast<Centavos>(-47850));
  VERIFICAR_IGUAL(diario->movimientos[2].efecto, static_cast<Centavos>(-800000));
}

PRUEBA(exportacion_describe_los_traspasos_desde_cada_lado) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);

  VERIFICAR_IGUAL(buscar(r, 1)->movimientos[2].concepto,
                  std::string("Traspaso hacia Casa"));
  VERIFICAR_IGUAL(buscar(r, 2)->movimientos[0].concepto,
                  std::string("Traspaso desde Gasto diario"));
}

PRUEBA(exportacion_pone_el_nombre_de_la_categoria) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);
  const ReporteDeSobre* diario = buscar(r, 1);
  VERIFICAR_IGUAL(diario->movimientos[0].categoria, std::string("Sueldo"));
  VERIFICAR_IGUAL(diario->movimientos[1].categoria, std::string("Comida"));
  // Un traspaso no lleva categoría.
  VERIFICAR_IGUAL(diario->movimientos[2].categoria, std::string(""));
}

PRUEBA(exportacion_deja_fuera_los_sobres_archivados_sin_actividad) {
  Escenario e;
  e.sobres.push_back(hacerSobre(3, "Viejo", TipoSobre::Propio, true));
  e.sobres.push_back(hacerSobre(4, "Vacío pero activo"));

  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);

  VERIFICAR(buscar(r, 3) == nullptr);
  // Un sobre activo aparece aunque no haya tenido movimientos.
  VERIFICAR(buscar(r, 4) != nullptr);
  VERIFICAR(buscar(r, 4)->movimientos.empty());
}

PRUEBA(exportacion_incluye_un_archivado_que_todavia_tiene_saldo) {
  Escenario e;
  e.sobres.push_back(hacerSobre(3, "Archivado con dinero", TipoSobre::Propio,
                                true));
  e.movimientos.push_back(mov(7, Fecha{2026, 7, 1}, TipoMovimiento::Entrada, 3,
                              kSinId, 11, 30000));

  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);
  VERIFICAR(buscar(r, 3) != nullptr);
  VERIFICAR_IGUAL(buscar(r, 3)->saldoInicial, static_cast<Centavos>(30000));
}

PRUEBA(exportacion_calcula_el_patrimonio_al_cierre_del_mes) {
  Escenario e;
  e.sobres.push_back(hacerSobre(3, "Roomies", TipoSobre::Ajeno));
  e.movimientos.push_back(mov(7, Fecha{2026, 9, 2}, TipoMovimiento::Entrada, 3,
                              kSinId, 11, 620000));
  Pasivo tarjeta;
  tarjeta.id = 1;
  tarjeta.nombre = "Tarjeta";
  tarjeta.saldo = 350000;
  e.pasivos.push_back(tarjeta);

  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);

  // 13,521.50 + 7,800 propios, 6,200 ajenos, 3,500 de deuda.
  VERIFICAR_IGUAL(r.patrimonioAlCierre.dineroPropio,
                  static_cast<Centavos>(2132150));
  VERIFICAR_IGUAL(r.patrimonioAlCierre.dineroAjeno,
                  static_cast<Centavos>(620000));
  VERIFICAR_IGUAL(r.patrimonioAlCierre.enLaCuenta,
                  static_cast<Centavos>(2752150));
  VERIFICAR_IGUAL(r.patrimonioAlCierre.patrimonioReal,
                  static_cast<Centavos>(1782150));
}

PRUEBA(exportacion_no_mezcla_meses) {
  Escenario e;
  const auto agosto = generarReporteMensualPorSobre(
      2026, 8, e.sobres, e.categorias, e.movimientos, e.pasivos);
  VERIFICAR_IGUAL(buscar(agosto, 1)->saldoInicial, static_cast<Centavos>(0));
  VERIFICAR_IGUAL(buscar(agosto, 1)->movimientos.size(),
                  static_cast<std::size_t>(2));
  VERIFICAR_IGUAL(buscar(agosto, 1)->saldoFinal, static_cast<Centavos>(400000));

  const auto octubre = generarReporteMensualPorSobre(
      2026, 10, e.sobres, e.categorias, e.movimientos, e.pasivos);
  VERIFICAR(buscar(octubre, 1)->movimientos.empty());
  VERIFICAR_IGUAL(buscar(octubre, 1)->saldoInicial,
                  static_cast<Centavos>(1352150));
}

PRUEBA(exportacion_arma_una_hoja_por_sobre_mas_el_resumen) {
  Escenario e;
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, e.movimientos, e.pasivos);
  const xlsx::Libro libro = construirLibroDeReporte(r);

  VERIFICAR_IGUAL(libro.hojas.size(), r.sobres.size() + 1);
  VERIFICAR_IGUAL(libro.hojas[0].nombre, std::string("Resumen"));
  VERIFICAR_IGUAL(libro.hojas[1].nombre, std::string("Gasto diario"));
  VERIFICAR_IGUAL(libro.hojas[2].nombre, std::string("Casa"));
}

PRUEBA(exportacion_no_repite_nombres_de_hoja) {
  Escenario e;
  // Dos sobres cuyo nombre se recorta al mismo texto de 31 caracteres.
  e.sobres = {hacerSobre(1, "Colegiatura de la universidad primera"),
              hacerSobre(2, "Colegiatura de la universidad segunda")};
  const auto r = generarReporteMensualPorSobre(
      2026, 9, e.sobres, e.categorias, {}, e.pasivos);
  const xlsx::Libro libro = construirLibroDeReporte(r);

  VERIFICAR_IGUAL(libro.hojas.size(), static_cast<std::size_t>(3));
  VERIFICAR(libro.hojas[1].nombre != libro.hojas[2].nombre);
  for (const xlsx::Hoja& h : libro.hojas) {
    VERIFICAR(h.nombre.size() <= 31);
  }
}

PRUEBA(exportacion_sugiere_el_nombre_del_archivo) {
  VERIFICAR_IGUAL(nombreDeArchivoSugerido(2026, 9),
                  std::string("Sobres 2026-09.xlsx"));
  VERIFICAR_IGUAL(nombreDeArchivoSugerido(2026, 12),
                  std::string("Sobres 2026-12.xlsx"));
}
