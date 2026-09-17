#include "../src/nucleo/saldos.h"

#include "marco.h"

using namespace sobres;

namespace {

Sobre hacerSobre(Id id, const char* nombre, TipoSobre tipo) {
  Sobre s;
  s.id = id;
  s.nombre = nombre;
  s.tipo = tipo;
  s.orden = static_cast<int>(id);
  return s;
}

Movimiento entrada(Id sobre, Centavos monto) {
  Movimiento m;
  m.tipo = TipoMovimiento::Entrada;
  m.sobreOrigen = sobre;
  m.monto = monto;
  m.fecha = Fecha{2026, 9, 1};
  return m;
}

Movimiento gasto(Id sobre, Centavos monto) {
  Movimiento m;
  m.tipo = TipoMovimiento::Gasto;
  m.sobreOrigen = sobre;
  m.monto = monto;
  m.fecha = Fecha{2026, 9, 2};
  return m;
}

Movimiento traspaso(Id de, Id a, Centavos monto) {
  Movimiento m;
  m.tipo = TipoMovimiento::Traspaso;
  m.sobreOrigen = de;
  m.sobreDestino = a;
  m.monto = monto;
  m.fecha = Fecha{2026, 9, 3};
  return m;
}

}  // namespace

PRUEBA(saldos_suman_entradas_gastos_y_traspasos) {
  std::vector<Sobre> sobres = {hacerSobre(1, "Gasto diario", TipoSobre::Propio),
                               hacerSobre(2, "Ahorro", TipoSobre::Meta)};
  std::vector<Movimiento> movimientos = {entrada(1, 500000), gasto(1, 120000),
                                         traspaso(1, 2, 100000)};

  const auto saldos = calcularSaldos(sobres, movimientos);
  VERIFICAR_IGUAL(saldos.at(1), static_cast<Centavos>(280000));
  VERIFICAR_IGUAL(saldos.at(2), static_cast<Centavos>(100000));
}

PRUEBA(saldos_ignoran_movimientos_de_sobres_que_ya_no_existen) {
  std::vector<Sobre> sobres = {hacerSobre(1, "Gasto diario", TipoSobre::Propio)};
  std::vector<Movimiento> movimientos = {entrada(1, 100000), gasto(99, 50000)};

  const auto saldos = calcularSaldos(sobres, movimientos);
  VERIFICAR_IGUAL(saldos.size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(saldos.at(1), static_cast<Centavos>(100000));
}

PRUEBA(patrimonio_deja_fuera_el_dinero_ajeno) {
  std::vector<Sobre> sobres = {
      hacerSobre(1, "Gasto diario", TipoSobre::Propio),
      hacerSobre(2, "Renta de los roomies", TipoSobre::Ajeno),
      hacerSobre(3, "Viaje", TipoSobre::Meta)};
  std::vector<Movimiento> movimientos = {entrada(1, 800000), entrada(2, 600000),
                                         entrada(3, 200000)};
  std::vector<Pasivo> pasivos;
  Pasivo tarjeta;
  tarjeta.id = 1;
  tarjeta.nombre = "Tarjeta de crédito";
  tarjeta.saldo = 350000;
  pasivos.push_back(tarjeta);

  const auto saldos = calcularSaldos(sobres, movimientos);
  const ResumenPatrimonio r = calcularPatrimonio(sobres, saldos, pasivos);

  VERIFICAR_IGUAL(r.enLaCuenta, static_cast<Centavos>(1600000));
  VERIFICAR_IGUAL(r.dineroAjeno, static_cast<Centavos>(600000));
  VERIFICAR_IGUAL(r.dineroPropio, static_cast<Centavos>(1000000));
  VERIFICAR_IGUAL(r.pasivos, static_cast<Centavos>(350000));
  // 10,000 propios menos 3,500 de tarjeta: 6,500 de patrimonio real, muy lejos
  // de los 16,000 que muestra el banco.
  VERIFICAR_IGUAL(r.patrimonioReal, static_cast<Centavos>(650000));
}

PRUEBA(patrimonio_puede_ser_negativo) {
  std::vector<Sobre> sobres = {hacerSobre(1, "Gasto diario", TipoSobre::Propio)};
  std::vector<Movimiento> movimientos = {entrada(1, 100000)};
  Pasivo deuda;
  deuda.id = 1;
  deuda.nombre = "Tarjeta";
  deuda.saldo = 450000;

  const auto saldos = calcularSaldos(sobres, movimientos);
  const ResumenPatrimonio r = calcularPatrimonio(sobres, saldos, {deuda});
  VERIFICAR_IGUAL(r.patrimonioReal, static_cast<Centavos>(-350000));
}

PRUEBA(patrimonio_explica_la_diferencia_en_palabras) {
  ResumenPatrimonio r;
  r.enLaCuenta = 1600000;
  r.dineroAjeno = 600000;
  r.dineroPropio = 1000000;
  r.pasivos = 350000;
  r.patrimonioReal = 650000;

  const std::string texto = explicarDiferencia(r);
  VERIFICAR(texto.find("$16,000.00") != std::string::npos);
  VERIFICAR(texto.find("$6,000.00") != std::string::npos);
  VERIFICAR(texto.find("$3,500.00") != std::string::npos);
  VERIFICAR(texto.find("$6,500.00") != std::string::npos);

  ResumenPatrimonio limpio;
  limpio.enLaCuenta = 100000;
  limpio.dineroPropio = 100000;
  limpio.patrimonioReal = 100000;
  VERIFICAR(explicarDiferencia(limpio).find("todo lo que está en la cuenta es "
                                            "tuyo") != std::string::npos);
}
