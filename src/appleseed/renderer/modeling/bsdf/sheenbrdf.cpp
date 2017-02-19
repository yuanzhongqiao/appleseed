
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "sheenbrdf.h"

// appleseed.renderer headers.
#include "renderer/kernel/lighting/scatteringmode.h"
#include "renderer/modeling/bsdf/bsdf.h"
#include "renderer/modeling/bsdf/bsdfwrapper.h"

// appleseed.foundation headers.
#include "foundation/math/basis.h"
#include "foundation/math/sampling/mappings.h"
#include "foundation/math/vector.h"
#include "foundation/utility/api/specializedapiarrays.h"
#include "foundation/utility/containers/dictionary.h"

// Standard headers.
#include <cmath>

// Forward declarations.
namespace foundation    { class IAbortSwitch; }
namespace renderer      { class Assembly; }
namespace renderer      { class Project; }

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // Sheen BRDF.
    //
    // References:
    //
    //   [1] Physically-Based Shading at Disney
    //       https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
    //

    const char* Model = "sheen_brdf";

    class SheenBRDFImpl
      : public BSDF
    {
      public:
        SheenBRDFImpl(
            const char*         name,
            const ParamArray&   params)
          : BSDF(name, Reflective, ScatteringMode::Diffuse, params)
        {
            m_inputs.declare("reflectance", InputFormatSpectralReflectance);
            m_inputs.declare("reflectance_multiplier", InputFormatFloat, "1.0");
        }

        virtual void release() APPLESEED_OVERRIDE
        {
            delete this;
        }

        virtual const char* get_model() const APPLESEED_OVERRIDE
        {
            return Model;
        }

        virtual void sample(
            SamplingContext&    sampling_context,
            const void*         data,
            const bool          adjoint,
            const bool          cosine_mult,
            BSDFSample&         sample) const APPLESEED_OVERRIDE
        {
            // No reflection below the shading surface.
            const float cos_on = dot(sample.m_outgoing.get_value(), sample.m_shading_basis.get_normal());
            if (cos_on < 0.0f)
                return;

            // Compute the incoming direction in local space.
            sampling_context.split_in_place(2, 1);
            const Vector2f s = sampling_context.next2<Vector2f>();
            const Vector3f wi = sample_hemisphere_uniform(s);

            // Transform the incoming direction to parent space.
            const Vector3f incoming = sample.m_shading_basis.transform_to_parent(wi);

            const Vector3f h = normalize(incoming + sample.m_outgoing.get_value());
            const float cos_ih = dot(incoming, h);
            const float fh = pow_int<5>(saturate(1.0f - cos_ih));

            const InputValues* values = static_cast<const InputValues*>(data);
            sample.m_value = values->m_reflectance;
            sample.m_value *= fh * values->m_reflectance_multiplier;

            sample.m_probability = RcpTwoPi<float>();

            sample.m_mode = ScatteringMode::Diffuse;
            sample.m_incoming = Dual3f(incoming);
            sample.compute_reflected_differentials();
        }

        virtual float evaluate(
            const void*         data,
            const bool          adjoint,
            const bool          cosine_mult,
            const Vector3f&     geometric_normal,
            const Basis3f&      shading_basis,
            const Vector3f&     outgoing,
            const Vector3f&     incoming,
            const int           modes,
            Spectrum&           value) const APPLESEED_OVERRIDE
        {
            if (!ScatteringMode::has_diffuse(modes))
                return 0.0f;

            // No reflection below the shading surface.
            const Vector3f& n = shading_basis.get_normal();
            const float cos_in = dot(incoming, n);
            const float cos_on = dot(outgoing, n);
            if (cos_in < 0.0f || cos_on < 0.0f)
                return 0.0f;

            const Vector3f h = normalize(incoming + outgoing);

            const float cos_ih = dot(incoming, h);
            const float fh = pow_int<5>(saturate(1.0f - cos_ih));

            const InputValues* values = static_cast<const InputValues*>(data);
            value = values->m_reflectance;
            value *= fh * values->m_reflectance_multiplier;

            return RcpTwoPi<float>();
        }

        virtual float evaluate_pdf(
            const void*         data,
            const Vector3f&     geometric_normal,
            const Basis3f&      shading_basis,
            const Vector3f&     outgoing,
            const Vector3f&     incoming,
            const int           modes) const APPLESEED_OVERRIDE
        {
            if (!ScatteringMode::has_diffuse(modes))
                return 0.0f;

            // No reflection below the shading surface.
            const Vector3f& n = shading_basis.get_normal();
            const float cos_in = dot(incoming, n);
            const float cos_on = dot(outgoing, n);
            if (cos_in < 0.0f || cos_on < 0.0f)
                return 0.0f;

            return RcpTwoPi<float>();
        }

      private:
        typedef SheenBRDFInputValues InputValues;
    };

    typedef BSDFWrapper<SheenBRDFImpl> SheenBRDF;
}


//
// SheenBRDFFactory class implementation.
//

const char* SheenBRDFFactory::get_model() const
{
    return Model;
}

Dictionary SheenBRDFFactory::get_model_metadata() const
{
    return
        Dictionary()
            .insert("name", Model)
            .insert("label", "Sheen BRDF");
}

DictionaryArray SheenBRDFFactory::get_input_metadata() const
{
    DictionaryArray metadata;

    metadata.push_back(
        Dictionary()
            .insert("name", "reflectance")
            .insert("label", "Reflectance")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.5"));

    metadata.push_back(
        Dictionary()
            .insert("name", "reflectance_multiplier")
            .insert("label", "Reflectance Multiplier")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary().insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    return metadata;
}

auto_release_ptr<BSDF> SheenBRDFFactory::create(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<BSDF>(new SheenBRDF(name, params));
}

auto_release_ptr<BSDF> SheenBRDFFactory::static_create(
    const char*         name,
    const ParamArray&   params)
{
    return auto_release_ptr<BSDF>(new SheenBRDF(name, params));
}

}   // namespace renderer
