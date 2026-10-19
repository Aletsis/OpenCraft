#include <cassert>
#include <iostream>
#include "content/ContentPackManager.hpp"

using namespace OpenCraft;

// A12: el hash de red identifica el contenido completo de los paquetes, no solo su manifiesto
int main() {
    ContentPack base;
    base.manifest.id = "hash_test";
    base.manifest.version = "1";
    base.recipes.push_back({"opencraft:diamond", 1, {"opencraft:dirt"}});
    base.functions["init"] = {"say hola"};

    auto hashOf = [](const ContentPack& p) {
        ContentPackManager m;
        bool ok = m.registerPack(p);
        assert(ok);
        (void)ok;
        m.enablePack(p.manifest.id);
        return m.computeNetworkHash();
    };

    std::string h0 = hashOf(base);
    assert(h0 == hashOf(base) && "El hash debe ser estable para el mismo contenido");

    ContentPack ingredients = base;
    ingredients.recipes[0].ingredients = {"opencraft:stone"};
    assert(hashOf(ingredients) != h0 && "Cambiar ingredientes debe cambiar el hash");

    ContentPack shape = base;
    shape.recipes[0].isShaped = true;
    shape.recipes[0].width = 1;
    shape.recipes[0].height = 1;
    assert(hashOf(shape) != h0 && "Cambiar la forma de la receta debe cambiar el hash");

    ContentPack body = base;
    body.functions["init"] = {"say adios"};
    assert(hashOf(body) != h0 && "Cambiar el cuerpo de una función debe cambiar el hash");

    std::cout << "EXITO: el hash de red cubre recetas, formas y funciones." << std::endl;
    return 0;
}
