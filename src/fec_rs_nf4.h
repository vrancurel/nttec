/* -*- mode: c++ -*- */
/*
 * Copyright 2017-2018 Scality
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __NTTEC_FEC_RS_NF4_H__
#define __NTTEC_FEC_RS_NF4_H__

#include <string>

#include "fec_base.h"
#include "fft_2n.h"
#include "gf_base.h"
#include "gf_nf4.h"
#include "polynomial.h"
#include "vec_vector.h"
#include "vec_zero_ext.h"

namespace nttec {
namespace fec {

/** Reed-Solomon (RS) Erasure code over `n` GF(F<sub>4</sub>). */
template <typename T>
class RsNf4 : public FecCode<T> {
  public:
    RsNf4(unsigned word_size, unsigned n_data, unsigned n_parities)
        : FecCode<T>(FecType::NON_SYSTEMATIC, word_size, n_data, n_parities)
    {
        this->fec_init();
    }

    ~RsNf4()
    {
        if (ngff4)
            delete ngff4;
    }

    inline void check_params() override
    {
        assert(this->word_size >= 2);
        assert(this->word_size <= 8);
    }

    inline void init_gf() override
    {
        gf_n = this->word_size / 2;

        ngff4 = new gf::NF4<T>(gf_n);
        this->gf = ngff4;

        sub_field = ngff4->get_sub_field();
    }

    inline void init_fft() override
    {
        // with this encoder we cannot exactly satisfy users request, we need to
        // pad n = minimal divisor of (q-1) that is at least (n_parities +
        // n_data)
        this->n =
            sub_field->get_code_len_high_compo(this->n_parities + this->n_data);

        // compute root of order n-1 such as r^(n-1) mod q == (1, ..,1)
        this->r = ngff4->get_nth_root(this->n);

        int m = arith::get_smallest_power_of_2<int>(this->n_data);
        this->fft = std::unique_ptr<fft::Radix2<T>>(
            new fft::Radix2<T>(ngff4, this->n, m));

        this->fft_full =
            std::unique_ptr<fft::Radix2<T>>(new fft::Radix2<T>(ngff4, this->n));

        unsigned len_2k = this->gf->get_code_len_high_compo(2 * this->n_data);
        this->fft_2k = std::unique_ptr<fft::Radix2<T>>(
            new fft::Radix2<T>(this->gf, len_2k, len_2k));
    }

    inline void init_others() override
    {
        // vector stores r^{-i} for i = 0, ... , k
        const T inv_r = ngff4->inv(this->r);
        this->inv_r_powers = std::unique_ptr<vec::Vector<T>>(
            new vec::Vector<T>(ngff4, this->n_data + 1));
        for (unsigned i = 0; i <= this->n_data; i++)
            this->inv_r_powers->set(i, ngff4->exp(inv_r, i));

        // vector stores r^{i} for i = 0, ... , k
        this->r_powers =
            std::unique_ptr<vec::Vector<T>>(new vec::Vector<T>(ngff4, this->n));
        for (int i = 0; i < this->n; i++)
            this->r_powers->set(i, ngff4->exp(this->r, i));
    }

    int get_n_outputs() override
    {
        return this->n;
    }

    /**
     * Encode vector
     *
     * @param output must be n
     * @param props must be exactly n
     * @param offset used to locate special values
     * @param words must be n_data
     */
    void encode(
        vec::Vector<T>* output,
        std::vector<Properties>& props,
        off_t offset,
        vec::Vector<T>* words) override
    {
        // std::cout << "words:"; words->dump();
        for (unsigned i = 0; i < this->n_data; i++) {
            words->set(i, ngff4->pack(words->get(i)));
        }
        // std::cout << "pack words:"; words->dump();
        vec::ZeroExtended<T> vwords(words, this->n);
        this->fft->fft(output, &vwords);
        // std::cout << "encoded:"; output->dump();
        for (unsigned i = 0; i < this->code_len; i++) {
            T val = output->get(i);
            GroupedValues<T> true_val = ngff4->unpack(val);
            if (true_val.flag > 0) {
                props[i].add(
                    ValueLocation(offset, i), std::to_string(true_val.flag));
                // std::cout << "\ni:" << true_val.flag << " at buf" << buf <<
                // std::endl; std::cout << "encode: val:" << val << " <- " <<
                // true_val.val << std::endl;
            }
            output->set(i, true_val.values);
        }
        // std::cout << "unpacked:"; output->dump();
    }

    void decode_add_data(int fragment_index, int row) override
    {
        // not applicable
        assert(false);
    }

    void decode_add_parities(int fragment_index, int row) override
    {
        // we can't anticipate here
    }

    void decode_build() override
    {
        // nothing to do
    }

  private:
    gf::Field<uint32_t>* sub_field;
    gf::NF4<T>* ngff4;
    int gf_n;

  protected:
    void decode_init(DecodeContext<T>* context, vec::Vector<T>* fragments_ids)
        override
    {
        if (this->inv_r_powers == nullptr) {
            throw LogicError("FEC base: vector (inv_r)^i must be initialized");
        }
        if (this->r_powers == nullptr) {
            throw LogicError("FEC base: vector r^i must be initialized");
        }
        if (this->fft == nullptr) {
            throw LogicError("FEC base: FFT must be initialized");
        }
        if (this->fft_full == nullptr) {
            throw LogicError("FEC base: FFT full must be initialized");
        }

        int k = this->n_data; // number of fragments received
        // vector x=(x_0, x_1, ..., x_k-1)
        vec::Vector<T> vx(this->gf, k);
        for (int i = 0; i < k; ++i) {
            vx.set(
                i,
                this->gf->exp(
                    this->r, ngff4->replicate(fragments_ids->get(i))));
        }

        // initialize context
        context->set_frag_ids(fragments_ids);

        vec::Poly<T>* A = context->get_A();
        vec::Poly<T>* inv_A_i = context->get_inv_A_i();

        // compute A(x) = prod_j(x-x_j)
        A->set(0, ngff4->get_unit());
        for (int i = 0; i < k; ++i) {
            A->mul_to_x_plus_coef(this->gf->sub(0, vx.get(i)));
        }

        // compute A'(x) since A_i(x_i) = A'_i(x_i)
        vec::Poly<T> _A(this->gf, this->n);
        _A.zero();
        for (int i = 1; i <= A->get_deg(); ++i)
            _A.set(i - 1, ngff4->mul(A->get(i), ngff4->replicate(i)));

        // compute A_i(x_i)
        this->fft->fft(inv_A_i, &_A);

        // compute 1/(x_i * A_i(x_i))
        // we care only about elements corresponding to fragments_ids
        for (unsigned i = 0; i < k; ++i) {
            unsigned j = fragments_ids->get(i);
            inv_A_i->set(
                j, this->gf->inv(this->gf->mul(inv_A_i->get(j), vx.get(i))));
        }

        // compute FFT(A) of length 2k
        unsigned len_2k = context->get_len_2k();
        vec::ZeroExtended<T> A_2k(A, len_2k);
        vec::Poly<T>* A_fft_2k = context->get_A_fft_2k();
        this->fft_2k->fft(A_fft_2k, &A_2k);
    }

    void decode_prepare(
        DecodeContext<T>* context,
        const std::vector<Properties>& props,
        off_t offset,
        vec::Vector<T>* words) override
    {
        T true_val;
        vec::Vector<T>* fragments_ids = context->get_frag_ids();
        // std::cout << "fragments_ids:"; fragments_ids->dump();
        int k = this->n_data; // number of fragments received
        for (int i = 0; i < k; ++i) {
            const int j = fragments_ids->get(i);
            auto data = props[j].get(ValueLocation(offset, j));

            if (data) {
                uint32_t flag = std::stoul(*data);
                true_val = ngff4->pack(words->get(i), flag);
            } else {
                true_val = ngff4->pack(words->get(i));
            }
            words->set(i, true_val);
        }
    }

    void decode_apply(
        DecodeContext<T>* context,
        vec::Vector<T>* output,
        vec::Vector<T>* words) override
    {
        // decode_apply: do the same thing as in fec_base
        FecCode<T>::decode_apply(context, output, words);
        // unpack decoded symbols
        for (unsigned i = 0; i < this->n_data; ++i) {
            output->set(i, ngff4->unpack(output->get(i)).values);
        }
    }
};

} // namespace fec
} // namespace nttec

#endif
