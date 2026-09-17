#include "../src/nucleo/recurrencia.h"

#include "marco.h"

using namespace sobres;

PRUEBA(recurrencia_avanza_semanas) {
  const Fecha f{2026, 9, 4};
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Semanal, 1) == (Fecha{2026, 9, 11}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Semanal, 4) == (Fecha{2026, 10, 2}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Semanal, -1) == (Fecha{2026, 8, 28}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Semanal, 0) == f);
}

PRUEBA(recurrencia_avanza_meses_recortando_el_dia) {
  VERIFICAR(avanzarPeriodos(Fecha{2026, 1, 31}, Periodicidad::Mensual, 1) ==
            (Fecha{2026, 2, 28}));
  VERIFICAR(avanzarPeriodos(Fecha{2026, 9, 4}, Periodicidad::Mensual, 12) ==
            (Fecha{2027, 9, 4}));
}

PRUEBA(recurrencia_avanza_quincenas_alternando) {
  const Fecha f{2026, 9, 1};
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Quincenal, 1) == (Fecha{2026, 9, 16}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Quincenal, 2) == (Fecha{2026, 10, 1}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Quincenal, 3) == (Fecha{2026, 10, 16}));
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Quincenal, 24) == (Fecha{2027, 9, 1}));
  // Retroceder una quincena es el reflejo exacto de avanzar una.
  VERIFICAR(avanzarPeriodos(f, Periodicidad::Quincenal, -2) == (Fecha{2026, 8, 1}));
}

PRUEBA(recurrencia_las_quincenas_siempre_avanzan) {
  Fecha anterior{2026, 1, 1};
  for (int k = 1; k <= 60; ++k) {
    const Fecha actual = avanzarPeriodos(Fecha{2026, 1, 1},
                                         Periodicidad::Quincenal, k);
    VERIFICAR(anterior < actual);
    anterior = actual;
  }
}

PRUEBA(recurrencia_cuenta_periodos_completos_entre_dos_fechas) {
  VERIFICAR_IGUAL(periodosEntre(Fecha{2026, 1, 1}, Fecha{2028, 1, 1},
                                Periodicidad::Mensual),
                  24LL);
  VERIFICAR_IGUAL(periodosEntre(Fecha{2026, 1, 1}, Fecha{2026, 1, 31},
                                Periodicidad::Mensual),
                  0LL);
  VERIFICAR_IGUAL(periodosEntre(Fecha{2026, 1, 1}, Fecha{2026, 2, 1},
                                Periodicidad::Mensual),
                  1LL);
  VERIFICAR_IGUAL(periodosEntre(Fecha{2026, 1, 1}, Fecha{2026, 1, 15},
                                Periodicidad::Semanal),
                  2LL);
  // Una fecha anterior no da periodos negativos.
  VERIFICAR_IGUAL(periodosEntre(Fecha{2026, 5, 1}, Fecha{2026, 1, 1},
                                Periodicidad::Mensual),
                  0LL);
}

PRUEBA(recurrencia_cuenta_las_materializaciones_pendientes) {
  Recurrente r;
  r.nombre = "Renta";
  r.periodicidad = Periodicidad::Mensual;
  r.proximaFecha = Fecha{2026, 6, 1};
  r.activo = true;

  // Antes de la fecha no hay nada que hacer.
  VERIFICAR_IGUAL(materializacionesPendientes(r, Fecha{2026, 5, 31}), 0LL);
  // El día exacto ya cuenta como vencido.
  VERIFICAR_IGUAL(materializacionesPendientes(r, Fecha{2026, 6, 1}), 1LL);
  // Tres meses después se acumularon cuatro.
  VERIFICAR_IGUAL(materializacionesPendientes(r, Fecha{2026, 9, 4}), 4LL);

  r.activo = false;
  VERIFICAR_IGUAL(materializacionesPendientes(r, Fecha{2026, 9, 4}), 0LL);
}
