#include "../src/nucleo/validacion.h"

#include "marco.h"

using namespace sobres;

namespace {

std::vector<Sobre> dosSobres() {
  Sobre a;
  a.id = 1;
  a.nombre = "Gasto diario";
  a.color = "#4b6bfb";
  Sobre b;
  b.id = 2;
  b.nombre = "Casa";
  b.color = "#0f766e";
  return {a, b};
}

}  // namespace

PRUEBA(validacion_acepta_un_color_hexadecimal) {
  VERIFICAR(esColorValido("#4b6bfb"));
  VERIFICAR(esColorValido("#FFFFFF"));
  VERIFICAR(!esColorValido("4b6bfb"));
  VERIFICAR(!esColorValido("#4b6bf"));
  VERIFICAR(!esColorValido("#zzzzzz"));
}

PRUEBA(validacion_rechaza_sobres_con_nombre_repetido) {
  const auto existentes = dosSobres();
  Sobre nuevo;
  nuevo.nombre = "  gasto DIARIO ";  // mismo nombre con otra caja y espacios
  nuevo.color = "#000000";
  VERIFICAR(!validarSobre(nuevo, existentes).empty());

  nuevo.nombre = "Colegiatura";
  VERIFICAR_IGUAL(validarSobre(nuevo, existentes), std::string(""));

  // Editar un sobre sin cambiarle el nombre no choca consigo mismo.
  Sobre editado = existentes[0];
  VERIFICAR_IGUAL(validarSobre(editado, existentes), std::string(""));
}

PRUEBA(validacion_exige_nombre_y_color) {
  const auto existentes = dosSobres();
  Sobre sinNombre;
  sinNombre.nombre = "   ";
  sinNombre.color = "#000000";
  VERIFICAR(!validarSobre(sinNombre, existentes).empty());

  Sobre malColor;
  malColor.nombre = "Otro";
  malColor.color = "azul";
  VERIFICAR(!validarSobre(malColor, existentes).empty());
}

PRUEBA(validacion_de_movimientos) {
  const auto sobres = dosSobres();
  Movimiento m;
  m.fecha = Fecha{2026, 9, 4};
  m.tipo = TipoMovimiento::Gasto;
  m.sobreOrigen = 1;
  m.categoria = 10;
  m.monto = 10000;
  VERIFICAR_IGUAL(validarMovimiento(m, sobres), std::string(""));

  m.monto = 0;
  VERIFICAR(!validarMovimiento(m, sobres).empty());

  m.monto = 10000;
  m.sobreOrigen = 99;
  VERIFICAR(!validarMovimiento(m, sobres).empty());

  m.sobreOrigen = 1;
  m.fecha = Fecha{2026, 2, 30};
  VERIFICAR(!validarMovimiento(m, sobres).empty());
}

PRUEBA(validacion_exige_categoria_en_gastos_y_entradas) {
  const auto sobres = dosSobres();
  Movimiento m;
  m.fecha = Fecha{2026, 9, 4};
  m.sobreOrigen = 1;
  m.monto = 10000;

  m.tipo = TipoMovimiento::Gasto;
  VERIFICAR(validarMovimiento(m, sobres).find("categoría") !=
            std::string::npos);
  m.tipo = TipoMovimiento::Entrada;
  VERIFICAR(validarMovimiento(m, sobres).find("categoría") !=
            std::string::npos);

  m.categoria = 10;
  VERIFICAR_IGUAL(validarMovimiento(m, sobres), std::string(""));

  // Un traspaso no gasta dinero, solo lo mueve, así que no lleva categoría.
  Movimiento traspaso;
  traspaso.fecha = Fecha{2026, 9, 4};
  traspaso.tipo = TipoMovimiento::Traspaso;
  traspaso.sobreOrigen = 1;
  traspaso.sobreDestino = 2;
  traspaso.monto = 10000;
  VERIFICAR_IGUAL(validarMovimiento(traspaso, sobres), std::string(""));
}

PRUEBA(validacion_exige_categoria_en_los_recurrentes) {
  const auto sobres = dosSobres();
  Recurrente r;
  r.nombre = "Renta";
  r.tipo = TipoMovimiento::Gasto;
  r.sobreOrigen = 1;
  r.monto = 850000;
  r.proximaFecha = Fecha{2026, 10, 1};
  VERIFICAR(!validarRecurrente(r, sobres).empty());

  r.categoria = 10;
  VERIFICAR_IGUAL(validarRecurrente(r, sobres), std::string(""));
}

PRUEBA(validacion_de_traspasos_exige_dos_sobres_distintos) {
  const auto sobres = dosSobres();
  Movimiento m;
  m.fecha = Fecha{2026, 9, 4};
  m.tipo = TipoMovimiento::Traspaso;
  m.sobreOrigen = 1;
  m.sobreDestino = 1;
  m.monto = 10000;
  VERIFICAR(!validarMovimiento(m, sobres).empty());

  m.sobreDestino = 2;
  VERIFICAR_IGUAL(validarMovimiento(m, sobres), std::string(""));

  m.sobreDestino = kSinId;
  VERIFICAR(!validarMovimiento(m, sobres).empty());
}

PRUEBA(validacion_de_pasivos_y_metas) {
  Pasivo p;
  p.nombre = "Tarjeta";
  p.saldo = 100000;
  VERIFICAR_IGUAL(validarPasivo(p), std::string(""));
  p.saldo = -1;
  VERIFICAR(!validarPasivo(p).empty());

  const auto sobres = dosSobres();
  Meta m;
  m.nombre = "Viaje";
  m.montoObjetivo = 1000000;
  m.fechaObjetivo = Fecha{2028, 1, 1};
  m.sobreAsociado = 1;
  m.aportePlaneado = 10000;
  m.tasaAnual = 0.08;
  VERIFICAR_IGUAL(validarMeta(m, sobres), std::string(""));

  m.tasaAnual = 50.0;  // 5000% anual
  VERIFICAR(!validarMeta(m, sobres).empty());

  m.tasaAnual = 0.08;
  m.sobreAsociado = 99;
  VERIFICAR(!validarMeta(m, sobres).empty());
}

PRUEBA(validacion_de_plantillas_de_reparto) {
  const auto sobres = dosSobres();
  PlantillaReparto p;
  p.nombre = "Sueldo";
  p.modo = ModoReparto::Porcentajes;
  p.lineas = {{1, 6000, false}, {2, 4000, false}};
  // Sin sobre fuente la plantilla genera entradas, y las entradas llevan
  // categoría.
  VERIFICAR(!validarPlantillaReparto(p, sobres).empty());
  p.categoria = 10;
  VERIFICAR_IGUAL(validarPlantillaReparto(p, sobres), std::string(""));

  p.lineas[1].valor = 3000;  // suman 90%
  VERIFICAR(!validarPlantillaReparto(p, sobres).empty());

  p.lineas[1].valor = 4000;
  p.sobreFuente = 1;  // el sobre fuente también es destino
  VERIFICAR(!validarPlantillaReparto(p, sobres).empty());

  p.sobreFuente = kSinId;
  p.lineas.clear();
  VERIFICAR(!validarPlantillaReparto(p, sobres).empty());
}
