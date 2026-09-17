#include "../src/nucleo/reparto.h"

#include "marco.h"

using namespace sobres;

namespace {

PlantillaReparto plantillaPorcentajes() {
  PlantillaReparto p;
  p.id = 1;
  p.nombre = "Sueldo quincenal";
  p.modo = ModoReparto::Porcentajes;
  p.montoSugerido = 1200000;  // $12,000.00
  p.lineas = {{1, 5000, false}, {2, 3000, false}, {3, 2000, false}};
  return p;
}

Centavos sumaDeAsignaciones(const ResultadoReparto& r) {
  Centavos total = 0;
  for (const AsignacionReparto& a : r.asignaciones) total += a.monto;
  return total;
}

}  // namespace

PRUEBA(reparto_divide_porcentajes_exactos) {
  const ResultadoReparto r = calcularReparto(plantillaPorcentajes(), 1200000);
  VERIFICAR(r.valido);
  VERIFICAR_IGUAL(r.asignaciones.size(), static_cast<std::size_t>(3));
  VERIFICAR_IGUAL(r.asignaciones[0].monto, static_cast<Centavos>(600000));
  VERIFICAR_IGUAL(r.asignaciones[1].monto, static_cast<Centavos>(360000));
  VERIFICAR_IGUAL(r.asignaciones[2].monto, static_cast<Centavos>(240000));
  VERIFICAR_IGUAL(sumaDeAsignaciones(r), static_cast<Centavos>(1200000));
}

PRUEBA(reparto_no_pierde_ni_un_centavo_al_redondear) {
  PlantillaReparto p;
  p.nombre = "Tercios";
  p.modo = ModoReparto::Porcentajes;
  // Tres partes de 33.33%, 33.33% y 33.34%.
  p.lineas = {{1, 3333, false}, {2, 3333, false}, {3, 3334, false}};

  const ResultadoReparto r = calcularReparto(p, 100);  // un peso
  VERIFICAR(r.valido);
  VERIFICAR_IGUAL(sumaDeAsignaciones(r), static_cast<Centavos>(100));

  // Un monto que no divide bien entre nada.
  const ResultadoReparto otro = calcularReparto(p, 1000001);
  VERIFICAR(otro.valido);
  VERIFICAR_IGUAL(sumaDeAsignaciones(otro), static_cast<Centavos>(1000001));
}

PRUEBA(reparto_da_el_sobrante_a_la_linea_marcada) {
  PlantillaReparto p;
  p.modo = ModoReparto::Porcentajes;
  p.nombre = "Con línea de resto";
  // La tercera línea es la que absorbe, aunque su residuo sea el más chico.
  p.lineas = {{1, 3333, false}, {2, 3334, false}, {3, 3333, true}};

  const ResultadoReparto r = calcularReparto(p, 1000);  // $10.00
  VERIFICAR(r.valido);
  VERIFICAR_IGUAL(sumaDeAsignaciones(r), static_cast<Centavos>(1000));
  // 333 + 333 + 333 = 999; el centavo que falta va a la línea marcada.
  VERIFICAR_IGUAL(r.asignaciones[2].monto, static_cast<Centavos>(334));
}

PRUEBA(reparto_recorre_muchos_montos_sin_perder_centavos) {
  const PlantillaReparto p = plantillaPorcentajes();
  for (Centavos monto = 1; monto < 5000; monto += 7) {
    const ResultadoReparto r = calcularReparto(p, monto);
    VERIFICAR(r.valido);
    VERIFICAR_IGUAL(sumaDeAsignaciones(r), monto);
  }
}

PRUEBA(reparto_rechaza_porcentajes_que_no_suman_cien) {
  PlantillaReparto p = plantillaPorcentajes();
  p.lineas[0].valor = 4000;  // ahora suman 90%
  const ResultadoReparto r = calcularReparto(p, 1200000);
  VERIFICAR(!r.valido);
  VERIFICAR(r.problema.find("90%") != std::string::npos);
}

PRUEBA(reparto_con_montos_fijos_usa_la_suma_de_las_lineas) {
  PlantillaReparto p;
  p.nombre = "Fijos";
  p.modo = ModoReparto::MontosFijos;
  p.lineas = {{1, 350000, false}, {2, 150000, false}};

  const ResultadoReparto r = calcularReparto(p, 999999);  // se ignora
  VERIFICAR(r.valido);
  VERIFICAR_IGUAL(r.total, static_cast<Centavos>(500000));
  VERIFICAR_IGUAL(r.asignaciones[0].monto, static_cast<Centavos>(350000));
}

PRUEBA(reparto_genera_entradas_cuando_no_hay_sobre_fuente) {
  const ResultadoReparto r = calcularReparto(plantillaPorcentajes(), 1200000);
  const auto movimientos = generarMovimientosDeReparto(
      plantillaPorcentajes(), r, Fecha{2026, 9, 15});

  VERIFICAR_IGUAL(movimientos.size(), static_cast<std::size_t>(3));
  for (const Movimiento& m : movimientos) {
    VERIFICAR(m.tipo == TipoMovimiento::Entrada);
    VERIFICAR(m.fecha == (Fecha{2026, 9, 15}));
    VERIFICAR(m.sobreDestino == kSinId);
    VERIFICAR(m.nota.find("Sueldo quincenal") != std::string::npos);
  }
}

PRUEBA(reparto_genera_traspasos_cuando_hay_sobre_fuente) {
  PlantillaReparto p = plantillaPorcentajes();
  p.sobreFuente = 9;
  const ResultadoReparto r = calcularReparto(p, 1200000);
  const auto movimientos =
      generarMovimientosDeReparto(p, r, Fecha{2026, 9, 15});

  VERIFICAR_IGUAL(movimientos.size(), static_cast<std::size_t>(3));
  for (const Movimiento& m : movimientos) {
    VERIFICAR(m.tipo == TipoMovimiento::Traspaso);
    VERIFICAR_IGUAL(m.sobreOrigen, static_cast<Id>(9));
  }
  VERIFICAR_IGUAL(movimientos[0].sobreDestino, static_cast<Id>(1));
}

PRUEBA(reparto_no_genera_movimientos_en_cero) {
  PlantillaReparto p;
  p.nombre = "Con una línea diminuta";
  p.modo = ModoReparto::Porcentajes;
  p.lineas = {{1, 9999, false}, {2, 1, false}};
  // Con un peso, la línea del 0.01% se lleva cero centavos.
  const ResultadoReparto r = calcularReparto(p, 100);
  const auto movimientos = generarMovimientosDeReparto(p, r, Fecha{2026, 9, 1});
  VERIFICAR_IGUAL(movimientos.size(), static_cast<std::size_t>(1));
}

PRUEBA(reparto_formatea_puntos_base_como_porcentaje) {
  VERIFICAR_IGUAL(formatearPuntosBase(10000), std::string("100%"));
  VERIFICAR_IGUAL(formatearPuntosBase(2550), std::string("25.5%"));
  VERIFICAR_IGUAL(formatearPuntosBase(3333), std::string("33.33%"));
  VERIFICAR_IGUAL(formatearPuntosBase(5), std::string("0.05%"));
}
