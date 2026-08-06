#include "core/include/core.h"
#include "scene/include/scene.h"

__device__ bool sample_cosine_brdf(
    SceneGraph*    scene,
    Ray&           ray,
    Ray&           r_in,
    Color&         atten,
    HitRecord&     rec,
    ScatterRecord& srec,
    Generator&     gen
) {
    Vector3 direction;
    float   pdf_value;

    if (scene->lights) {
        Hittable** lights = reinterpret_cast<Hittable**>(scene->lights);
        MixturePDF pdf(srec.pdf, PDF::hittable(lights, rec.p));
        direction = pdf.generate(gen);
        pdf_value = pdf.value(direction);
    } else {
        direction = srec.pdf.generate(gen);
        pdf_value = srec.pdf.value(direction);
    }

    if (pdf_value <= 0.0f) return false;
    
    ray = Ray(rec.p, direction, r_in.time());
    Color brdf = rec.mat->evaluate(
        rec, normalize(-r_in.direction()), direction
    );
    atten *= brdf / pdf_value;

    return true;
}

