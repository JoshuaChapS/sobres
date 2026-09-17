#include "saldos.h"

namespace sobres {

std::map<Id, Centavos> calcularSaldos(
    const std::vector<Sobre>& sobres,
    const std::vector<Movimiento>& movimientos) {
  std::map<Id, Centavos> saldos;
  for (const Sobre& s : sobres) saldos[s.id] = 0;

  for (const Movimiento& m : movimientos) {
    switch (m.tipo) {
      case TipoMovimiento::Entrada:
        if (saldos.count(m.sobreOrigen)) saldos[m.sobreOrigen] += m.monto;
        break;
      case TipoMovimiento::Gasto:
        if (saldos.count(m.sobreOrigen)) saldos[m.sobreOrigen] -= m.monto;
        break;
      case TipoMovimiento::Traspaso:
        if (saldos.count(m.sobreOrigen)) saldos[m.sobreOrigen] -= m.monto;
        if (saldos.count(m.sobreDestino)) saldos[m.sobreDestino] += m.monto;
        break;
    }
  }
  return saldos;
}

ResumenPatrimonio calcularPatrimonio(const std::vector<Sobre>& sobres,
                                     const std::map<Id, Centavos>& saldos,
                                     const std::vector<Pasivo>& pasivos) {
  ResumenPatrimonio r;
  for (const Sobre& s : sobres) {
    auto it = saldos.find(s.id);
    if (it == saldos.end()) continue;
    const Centavos saldo = it->second;
    r.enLaCuenta += saldo;
    if (cuentaParaPatrimonio(s.tipo)) {
      r.dineroPropio += saldo;
    } else {
      r.dineroAjeno += saldo;
    }
  }
  for (const Pasivo& p : pasivos) r.pasivos += p.saldo;
  r.patrimonioReal = r.dineroPropio - r.pasivos;
  return r;
}

std::string explicarDiferencia(const ResumenPatrimonio& r) {
  const bool hayAjeno = r.dineroAjeno != 0;
  const bool hayPasivos = r.pasivos != 0;

  if (!hayAjeno && !hayPasivos) {
    return "No hay dinero de terceros ni deudas registradas, así que todo lo "
           "que está en la cuenta es tuyo.";
  }

  std::string texto = "De los " + formatearPesos(r.enLaCuenta) +
                      " que hay en la cuenta, ";
  if (hayAjeno && hayPasivos) {
    texto += formatearPesos(r.dineroAjeno) +
             " son de terceros y solo están de paso, y " +
             formatearPesos(r.pasivos) + " ya le pertenecen a lo que debes.";
  } else if (hayAjeno) {
    texto += formatearPesos(r.dineroAjeno) +
             " son de terceros y solo están de paso.";
  } else {
    texto += formatearPesos(r.pasivos) + " ya le pertenecen a lo que debes.";
  }
  texto += " Lo que de verdad es tuyo son " +
           formatearPesos(r.patrimonioReal) + ".";
  return texto;
}

}  // namespace sobres
