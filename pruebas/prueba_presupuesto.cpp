#include "../src/nucleo/presupuesto.h"
#include "marco.h"

using namespace sobres;

PRUEBA(presupuesto_disponible) {
  VERIFICAR_IGUAL(disponible({100000, 80000}), static_cast<Centavos>(20000));
  VERIFICAR_IGUAL(disponible({100000, 100000}), static_cast<Centavos>(0));
  VERIFICAR_IGUAL(disponible({100000, 150000}), static_cast<Centavos>(-50000));
  VERIFICAR_IGUAL(disponible({0, 0}), static_cast<Centavos>(0));
}

PRUEBA(presupuesto_porcentaje_basico) {
  VERIFICAR_IGUAL(porcentajeUsado({100000, 80000}), 80);
  VERIFICAR_IGUAL(porcentajeUsado({100000, 0}), 0);
  VERIFICAR_IGUAL(porcentajeUsado({100000, 100000}), 100);
  VERIFICAR_IGUAL(porcentajeUsado({100000, 150000}), 150);
}

PRUEBA(presupuesto_porcentaje_redondea_al_mas_cercano) {
  VERIFICAR_IGUAL(porcentajeUsado({300, 100}), 33);   // 33.33 -> 33
  VERIFICAR_IGUAL(porcentajeUsado({300, 200}), 67);   // 66.67 -> 67
  VERIFICAR_IGUAL(porcentajeUsado({800, 4}), 1);      // 0.5   -> 1  (se aleja del cero)
}

PRUEBA(presupuesto_porcentaje_con_asignado_en_cero) {
  VERIFICAR_IGUAL(porcentajeUsado({0, 0}), 0);
  VERIFICAR_IGUAL(porcentajeUsado({0, 1}), 100);
  VERIFICAR_IGUAL(porcentajeUsado({0, 500000}), 100);
}

PRUEBA(presupuesto_estado_en_las_fronteras) {
  VERIFICAR(evaluar({10000, 7900}) == EstadoPresupuesto::Sano);         // 79
  VERIFICAR(evaluar({10000, 8000}) == EstadoPresupuesto::Advertencia);  // 80
  VERIFICAR(evaluar({10000, 10000}) == EstadoPresupuesto::Advertencia); // 100
  VERIFICAR(evaluar({10000, 10100}) == EstadoPresupuesto::Excedido);    // 101
  VERIFICAR(evaluar({0, 0}) == EstadoPresupuesto::Sano);                // 0
  VERIFICAR(evaluar({0, 1}) == EstadoPresupuesto::Advertencia);         // 100
}

PRUEBA(presupuesto_resumen_tiene_el_formato_exacto) {
  VERIFICAR_IGUAL(resumen({100000, 80000}),
                  std::string("Gastado $800.00 de $1,000.00 (80%) - advertencia"));
  VERIFICAR_IGUAL(resumen({100000, 5000}),
                  std::string("Gastado $50.00 de $1,000.00 (5%) - sano"));
  VERIFICAR_IGUAL(resumen({100000, 150000}),
                  std::string("Gastado $1,500.00 de $1,000.00 (150%) - excedido"));
}

PRUEBA(presupuesto_mas_presionados_ordena_por_porcentaje) {
  std::vector<Presupuesto> ps = {
      {10000, 2000},   // 0 -> 20%
      {10000, 9500},   // 1 -> 95%
      {10000, 5000},   // 2 -> 50%
      {10000, 12000},  // 3 -> 120%
  };
  const std::vector<std::size_t> esperado = {3, 1, 2};
  VERIFICAR(masPresionados(ps, 3) == esperado);
}

PRUEBA(presupuesto_mas_presionados_desempata_por_gastado) {
  std::vector<Presupuesto> ps = {
      {10000, 5000},   // 0 -> 50%, gastó 5000
      {20000, 10000},  // 1 -> 50%, gastó 10000  <- va primero
  };
  const std::vector<std::size_t> esperado = {1, 0};
  VERIFICAR(masPresionados(ps, 2) == esperado);
}

PRUEBA(presupuesto_mas_presionados_bordes) {
  std::vector<Presupuesto> ps = {{10000, 5000}, {10000, 8000}};
  VERIFICAR(masPresionados(ps, 0).empty());
  VERIFICAR(masPresionados(ps, 99).size() == 2u);   // pide más de los que hay
  VERIFICAR(masPresionados({}, 3).empty());         // lista vacía
}
