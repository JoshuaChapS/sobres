// Sobres — validacion.h
// Reglas que un dato tiene que cumplir antes de guardarse. Viven en el núcleo
// para que la interfaz y la capa de datos apliquen exactamente las mismas.
#pragma once

#include <string>
#include <vector>

#include "modelos.h"

namespace sobres {

// Devuelve el texto del problema, o una cadena vacía si el dato es válido.
std::string validarSobre(const Sobre& s, const std::vector<Sobre>& existentes);
std::string validarCategoria(const Categoria& c,
                             const std::vector<Categoria>& existentes);
std::string validarMovimiento(const Movimiento& m,
                              const std::vector<Sobre>& sobres);
std::string validarPasivo(const Pasivo& p);
std::string validarMeta(const Meta& m, const std::vector<Sobre>& sobres);
std::string validarRecurrente(const Recurrente& r,
                              const std::vector<Sobre>& sobres);
std::string validarPlantillaReparto(const PlantillaReparto& p,
                                    const std::vector<Sobre>& sobres);

// Comprueba que un texto tenga la forma #RRGGBB.
bool esColorValido(const std::string& color);

}  // namespace sobres
