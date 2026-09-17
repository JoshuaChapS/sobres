// Sobres — estado.h
// El estado que comparten todas las pantallas: el repositorio abierto y una
// copia en memoria de lo que hay en él, ya con los saldos calculados. Cuando
// algo cambia se recarga todo de golpe y se avisa con una señal; con el volumen
// de datos de unas finanzas personales eso es instantáneo y evita cualquier
// posibilidad de que una pantalla muestre números viejos.
#pragma once

#include <QObject>
#include <QString>
#include <map>
#include <vector>

#include "../datos/repositorio.h"
#include "../nucleo/saldos.h"

namespace sobres {

class Estado : public QObject {
  Q_OBJECT

 public:
  explicit Estado(Repositorio* repositorio, QObject* padre = nullptr);

  Repositorio& repositorio() { return *repositorio_; }

  void recargar();

  const std::vector<Sobre>& sobres() const { return sobres_; }
  const std::vector<Sobre>& sobresConArchivados() const { return todosLosSobres_; }
  const std::vector<Categoria>& categorias() const { return categorias_; }
  const std::vector<Categoria>& categoriasConArchivadas() const {
    return todasLasCategorias_;
  }
  const std::vector<Movimiento>& movimientos() const { return movimientos_; }
  const std::vector<Pasivo>& pasivos() const { return pasivos_; }
  const std::vector<Meta>& metas() const { return metas_; }
  const std::vector<Recurrente>& recurrentes() const { return recurrentes_; }
  const std::vector<PlantillaReparto>& plantillas() const { return plantillas_; }

  const std::map<Id, Centavos>& saldos() const { return saldos_; }
  Centavos saldoDe(Id sobre) const;
  const ResumenPatrimonio& resumen() const { return resumen_; }

  // Devuelve el identificador de la categoría con ese nombre, creándola si no
  // existe y desarchivándola si estaba archivada. Con un nombre vacío devuelve
  // kSinId sin crear nada. No recarga el estado: eso lo hace quien guarda.
  Id asegurarCategoria(const QString& nombre, QString* error);

  QString nombreDeSobre(Id id) const;
  QString nombreDeCategoria(Id id) const;
  QString colorDeSobre(Id id) const;
  bool estaVacia() const { return todosLosSobres_.empty(); }

  // Fecha de hoy tomada del reloj del sistema.
  static Fecha hoy();

 signals:
  // La emite cualquier cambio en los datos. Las pantallas se vuelven a dibujar
  // cuando la reciben.
  void cambio();

 private:
  Repositorio* repositorio_;
  std::vector<Sobre> sobres_;
  std::vector<Sobre> todosLosSobres_;
  std::vector<Categoria> categorias_;
  std::vector<Categoria> todasLasCategorias_;
  std::vector<Movimiento> movimientos_;
  std::vector<Pasivo> pasivos_;
  std::vector<Meta> metas_;
  std::vector<Recurrente> recurrentes_;
  std::vector<PlantillaReparto> plantillas_;
  std::map<Id, Centavos> saldos_;
  ResumenPatrimonio resumen_;
};

}  // namespace sobres
