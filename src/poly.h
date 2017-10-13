/* -*- mode: c++ -*- */

#pragma once

template<typename T>
class Vec;

template<typename T>
class Poly
{
private:
  struct Term: std::map <int, T> {};
  RN<T> *rn;
  GF<T> *sub_field;
  Term terms;

 public:
  explicit Poly(RN<T> *rn);
  explicit Poly(GF<T> *gf);
  void clear();
  void copy(Poly<T> *src);
  void copy(Poly<T> *src, T offset);
  int degree();
  T lead();
  bool is_zero();
  T get(int exponent);
  void set(int exponent, T coef);
  void _neg(Poly<T> *result, Poly<T> *a);
  void _add(Poly<T> *result, Poly<T> *a, Poly<T> *b);
  void _sub(Poly<T> *result, Poly<T> *a, Poly<T> *b);
  void _mul(Poly<T> *result, Poly<T> *a, Poly<T> *b);
  void _div(Poly<T> *q, Poly<T> *r, Poly<T> *n, Poly<T> *d);
  void _derivative(Poly<T> *result, Poly<T> *a);
  void neg();
  void add(Poly<T> *b);
  void sub(Poly<T> *b);
  void mul(Poly<T> *b);
  void div(Poly<T> *d);
  void mod(Poly<T> *d);
  void derivative();
  T eval(T x);
  bool equal(Poly<T> *f);
  void to_vec(Vec<T> *vec);
  T to_num();
  void from_num(T x, int max_deg);
  void dump();
};

template <typename T>
Poly<T>::Poly(RN<T> *rn)
{
  this->rn = rn;
  this->sub_field = NULL;
}

template <typename T>
Poly<T>::Poly(GF<T> *gf)
{
  this->rn = gf;
  this->sub_field = gf->get_sub_field();
}

template <typename T>
void Poly<T>::clear()
{
  terms.clear();
}

template <typename T>
void Poly<T>::copy(Poly<T> *src)
{
  clear();

  for (int i = src->degree(); i >= 0; i--)
    set(i, src->get(i));
}

template <typename T>
void Poly<T>::copy(Poly<T> *src, T offset)
{
  clear();

  for (int i = src->degree(); i >= 0; i--)
    set(i + offset, src->get(i));
}

template <typename T>
int Poly<T>::degree()
{
  return (terms.rbegin() == terms.rend()) ? 0 : terms.rbegin()->first;
}

template <typename T>
T Poly<T>::lead()
{
  return get(degree());
}

template <typename T>
bool Poly<T>::is_zero()
{
  return degree() == 0 && get(0) == 0;
}

template <typename T>
T Poly<T>::get(int exponent)
{
  typename Term::const_iterator it = terms.find(exponent);

  return (it == terms.end()) ? 0 : it->second;
}

template <typename T>
void Poly<T>::set(int exponent, T coef)
{
  assert(rn->check(coef));

  typename Term::const_iterator it = terms.find(exponent);

  if (it == terms.end() && coef == 0)
    return;
  terms[exponent] = coef;
}

template <typename T>
void Poly<T>::_neg(Poly<T> *result, Poly<T> *a)
{
  result->clear();

  Poly<T> b(rn);
  sub(result, &b, a);
}

template <typename T>
void Poly<T>::_add(Poly<T> *result, Poly<T> *a, Poly<T> *b)
{
  result->clear();

  int max = std::max(a->degree(), b->degree());

  for (int i = max; i >= 0; i--)
    result->set(i,
                rn->add(a->get(i), b->get(i)));
}

template <typename T>
void Poly<T>::_sub(Poly<T> *result, Poly<T> *a, Poly<T> *b)
{
  result->clear();

  int max = std::max(a->degree(), b->degree());

  for (int i = max; i >= 0; i--)
    result->set(i,
                rn->sub(a->get(i), b->get(i)));
}

template <typename T>
void Poly<T>::_mul(Poly<T> *result, Poly<T> *a, Poly<T> *b)
{
  result->clear();

  for (int i = a->degree(); i >= 0; i--)
    for (int j = b->degree(); j >= 0; j--)
      result->set(i + j,
                  rn->add(result->get(i + j),
                          rn->mul(a->get(i), b->get(j))));
}

/**
 * long division algorithm (source Wikipedia)
 *
 * @param q quotient
 * @param r remainder
 * @param n dividend
 * @param d divisor
 */
template <typename T>
void Poly<T>::_div(Poly<T> *q, Poly<T> *r, Poly<T> *n, Poly<T> *d)
{
  Poly<T> _q(rn), _r(rn);

  if (d->is_zero())
    throw NTL_EX_DIV_BY_ZERO;

  _q.clear();
  _r.copy(n);
  while (!_r.is_zero() && (_r.degree() >= d->degree())) {
    Poly<T> _t(rn);
    _t.set(_r.degree() - d->degree(),
           rn->div(_r.lead(), d->lead()));
    _q.add(&_t);
    _t.mul(d);
    _r.sub(&_t);
  }

  if (q != nullptr)
    q->copy(&_q);

  if (r != nullptr)
    r->copy(&_r);
}

template <typename T>
void Poly<T>::_derivative(Poly<T> *result, Poly<T> *a)
{
  T _card;

  if (sub_field)
    _card = sub_field->card();
  else
    _card = rn->card();

  result->clear();

  for (int i = a->degree(); i > 0; i--)
    result->set(i - 1, rn->mul(a->get(i), i % _card));
}

template <typename T>
void Poly<T>::neg()
{
  Poly<T> a(rn), b(rn);
  b.copy(this);
  _sub(this, &a, &b);
}

template <typename T>
void Poly<T>::add(Poly<T> *b)
{
  Poly<T> a(rn);
  a.copy(this);
  _add(this, &a, b);
}

template <typename T>
void Poly<T>::sub(Poly<T> *b)
{
  Poly<T> a(rn);
  a.copy(this);
  _sub(this, &a, b);
}

template <typename T>
void Poly<T>::mul(Poly<T> *b)
{
  Poly<T> a(rn);
  a.copy(this);
  _mul(this, &a, b);
}

template <typename T>
void Poly<T>::div(Poly<T> *b)
{
  Poly<T> a(rn);
  a.copy(this);
  _div(this, NULL, &a, b);
}

template <typename T>
void Poly<T>::mod(Poly<T> *b)
{
  Poly<T> a(rn);
  a.copy(this);
  _div(NULL, this, &a, b);
}

template <typename T>
void Poly<T>::derivative()
{
  Poly<T> a(rn);
  a.copy(this);
  _derivative(this, &a);
}

template <typename T>
T Poly<T>::eval(T x)
{
  int i = degree();
  T result = get(i);

  while (i >= 1) {
    result = rn->add(rn->mul(result, x), get(i - 1));
    i--;
  }
  return result;
}

template <typename T>
bool Poly<T>::equal(Poly<T> *f) {
  T deg = degree();
  if (deg != f->degree())
    return false;
  for (T i = 0; i <= deg; i++)
    if (get(i) != f->get(i))
      return false;
  return true;
}

template <typename T>
void Poly<T>::to_vec(Vec<T> *vec) {
  T deg = degree();
  assert(vec->get_n() > deg);
  int i;
  for (i = 0; i <= deg; i++)
    vec->set(i, get(i));
  for (; i < vec->get_n(); i++)
    vec->set(i, 0);
}

/** 
 * convert a polynomial to numerical representation
 * 
 * 
 * @return 
 */
template <typename T>
T Poly<T>::to_num()
{
  int i = degree();
  T result = 0;
  
  while (i >= 0) {
    result += get(i) * exp<T>(rn->card(), i);
    i--;
  }
  return result;
}

/** 
 * convert a numerical representation of a polynomial
 * 
 * @param x 
 */
template <typename T>
void Poly<T>::from_num(T x, int max_deg)
{
  for (int i = max_deg;i >= 0;i--) {
    T tmp = exp<T>(rn->card(), i);
    T tmp2 = x / tmp;
    if (tmp2 != 0)
      set(i, tmp2);
    x -= tmp2 * tmp;
  }
}

template <typename T>
void Poly<T>::dump()
{
  typename Term::const_reverse_iterator it = terms.rbegin();

  for (; it != terms.rend(); it++)
    std::cout << " " << it->second << "x^" << it->first;
  std::cout << "\n";
}
