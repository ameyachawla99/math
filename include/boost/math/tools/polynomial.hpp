//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_POLYNOMIAL_HPP
#define BOOST_MATH_TOOLS_POLYNOMIAL_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/tools/real_cast.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/operators.hpp>

#include <vector>
#include <ostream>
#include <algorithm>

namespace boost{ namespace math{ namespace tools{

template <class T>
struct identity
{
    T operator()(T const &x) const
    {
        return x;
    }
};

template <class T>
T chebyshev_coefficient(unsigned n, unsigned m)
{
   BOOST_MATH_STD_USING
   if(m > n)
      return 0;
   if((n & 1) != (m & 1))
      return 0;
   if(n == 0)
      return 1;
   T result = T(n) / 2;
   unsigned r = n - m;
   r /= 2;

   BOOST_ASSERT(n - 2 * r == m);

   if(r & 1)
      result = -result;
   result /= n - r;
   result *= boost::math::binomial_coefficient<T>(n - r, r);
   result *= ldexp(1.0f, m);
   return result;
}

template <class Seq>
Seq polynomial_to_chebyshev(const Seq& s)
{
   // Converts a Polynomial into Chebyshev form:
   typedef typename Seq::value_type value_type;
   typedef typename Seq::difference_type difference_type;
   Seq result(s);
   difference_type order = s.size() - 1;
   difference_type even_order = order & 1 ? order - 1 : order;
   difference_type odd_order = order & 1 ? order : order - 1;

   for(difference_type i = even_order; i >= 0; i -= 2)
   {
      value_type val = s[i];
      for(difference_type k = even_order; k > i; k -= 2)
      {
         val -= result[k] * chebyshev_coefficient<value_type>(static_cast<unsigned>(k), static_cast<unsigned>(i));
      }
      val /= chebyshev_coefficient<value_type>(static_cast<unsigned>(i), static_cast<unsigned>(i));
      result[i] = val;
   }
   result[0] *= 2;

   for(difference_type i = odd_order; i >= 0; i -= 2)
   {
      value_type val = s[i];
      for(difference_type k = odd_order; k > i; k -= 2)
      {
         val -= result[k] * chebyshev_coefficient<value_type>(static_cast<unsigned>(k), static_cast<unsigned>(i));
      }
      val /= chebyshev_coefficient<value_type>(static_cast<unsigned>(i), static_cast<unsigned>(i));
      result[i] = val;
   }
   return result;
}

template <class Seq, class T>
T evaluate_chebyshev(const Seq& a, const T& x)
{
   // Clenshaw's formula:
   typedef typename Seq::difference_type difference_type;
   T yk2 = 0;
   T yk1 = 0;
   T yk = 0;
   for(difference_type i = a.size() - 1; i >= 1; --i)
   {
      yk2 = yk1;
      yk1 = yk;
      yk = 2 * x * yk1 - yk2 + a[i];
   }
   return a[0] / 2 + yk * x - yk1;
}


template <typename T>
class polynomial;


template <typename T>
std::pair< polynomial<T>, polynomial<T> >
unchecked_synthetic_division(const polynomial<T>& dividend, const polynomial<T>& divisor)
{
    BOOST_ASSERT(divisor.degree() <= dividend.degree());
    BOOST_ASSERT(divisor != zero_element(std::multiplies< polynomial<T> >()));
    BOOST_ASSERT(dividend != zero_element(std::multiplies< polynomial<T> >()));
    
    std::vector<T> intermediate_result(dividend.data());
    if (divisor.degree() == T(0))
        intermediate_result.insert(begin(intermediate_result), T(0));
    
    {
        typedef BOOST_DEDUCED_TYPENAME std::vector<T>::reverse_iterator reverse_iterator;
        T const normalizer = divisor[divisor.size() - 1];
        reverse_iterator const last = intermediate_result.rbegin() + dividend.degree() - divisor.degree() + 1;
        for (reverse_iterator i = intermediate_result.rbegin(); i != last; i++)
        {
            if (*i != T(0))
            {
                T const coefficient = *i /= normalizer;
                // for (std::size_t j = 1; j < divisor.degree() + 1; j++)
                    // *(i + j) -= *(divisor.data().rbegin() + j) * coefficient;
                boost::function<T(T, T)> const op = bind(std::minus<T>(), _1, bind(std::multiplies<T>(), _2, coefficient));
                reverse_iterator const j = i + 1;
                std::transform(j, j + divisor.degree(), divisor.data().rbegin() + 1, j, op);
            }
        }
    }
    
    {
        typedef BOOST_DEDUCED_TYPENAME std::vector<T>::iterator iterator;
        iterator const f = intermediate_result.begin(); // remainder
        iterator const m = f + std::max(divisor.degree(), 1ul); // quotient
        iterator const l = intermediate_result.end();
        BOOST_ASSERT(m - f > 0);
        BOOST_ASSERT(l - m > 0);
        polynomial<T> const quotient(m, l);
        polynomial<T> const remainder = *f != T(0) ? polynomial<T>(f, m) : polynomial<T>();
        return std::make_pair(quotient, remainder);
    }
}


/**
 * Returns the zero element for multiplication of polynomials.
 */
template <class T>
polynomial<T> zero_element(std::multiplies< polynomial<T> >)
{
    return polynomial<T>();
}

/* Calculates a / b and a % b, returning the pair (quotient, remainder) together
 * because the same amount of computation yields both.
 */
template <typename T>
std::pair< polynomial<T>, polynomial<T> >
quotient_remainder(const polynomial<T>& dividend, const polynomial<T>& divisor)
{
    polynomial<T> const zero = zero_element(std::multiplies< polynomial<T> >());
    if (divisor == zero)
        throw std::domain_error("Divide by zero.");
    if (dividend.degree() < divisor.degree())
        return std::make_pair(zero, dividend);
    return unchecked_synthetic_division(dividend, divisor);
}


template <class T>
class polynomial : 
    equality_comparable< polynomial<T>, 
    dividable< polynomial<T>, 
    modable< polynomial<T> > > >
{
public:
   // typedefs:
   typedef typename std::vector<T>::value_type value_type;
   typedef typename std::vector<T>::size_type size_type;

   // construct:
   polynomial(){}
   
   template <class U>
   polynomial(const U* data, unsigned order)
      : m_data(data, data + order + 1)
   {
   }
   
   template <class I>
   polynomial(I first, I last)
   : m_data(first, last)
   {
   }
   
   template <class U>
   polynomial(const U& point)
   {
       if (point != U(0))
          m_data.push_back(point);
   }

   // copy:
   polynomial(const polynomial& p)
      : m_data(p.m_data) { }

   template <class U>
   polynomial(const polynomial<U>& p)
   {
      for(unsigned i = 0; i < p.size(); ++i)
      {
         m_data.push_back(boost::math::tools::real_cast<T>(p[i]));
      }
   }

   // access:
   size_type size()const { return m_data.size(); }
   size_type degree()const
   { 
       BOOST_ASSERT(size() != 0);
       return m_data.size() - 1;
    }
   value_type& operator[](size_type i)
   {
      return m_data[i];
   }
   const value_type& operator[](size_type i)const
   {
      return m_data[i];
   }
   T evaluate(T z)const
   {
      return boost::math::tools::evaluate_polynomial(&m_data[0], z, m_data.size());;
   }
   std::vector<T> chebyshev()const
   {
      return polynomial_to_chebyshev(m_data);
   }
   
   std::vector<T> const& data() const
   {
       return m_data;
   }

   // operators:
   template <class U>
   polynomial& operator +=(const U& value)
   {
      if(m_data.size() == 0)
         m_data.push_back(value);
      else
      {
         m_data[0] += value;
      }
      return *this;
   }
   template <class U>
   polynomial& operator -=(const U& value)
   {
       if (value != U(0))
       {
            if(m_data.size() == 0)
                m_data.push_back(-value);
            else
            {
                if (m_data.size() == 1 && m_data[0] == value)
                    m_data.clear();
                else
                    m_data[0] -= value;
            }
       }
      return *this;
   }
   template <class U>
   polynomial& operator *=(const U& value)
   {
      for(size_type i = 0; i < m_data.size(); ++i)
         m_data[i] *= value;
      // std::transform(m_data.begin(), m_data.end(), m_data.begin(), bind(std::multiplies<U>(), _1, value));
      return *this;
   }
   
   template <class U>
   polynomial& operator +=(const polynomial<U>& value)
   {
      return linear_translation(value, identity<U>(), std::plus<U>());
   }
   
   template <class U>
   polynomial& operator -=(const polynomial<U>& value)
   {
       return linear_translation(value, std::negate<U>(), std::minus<U>());
   }
   template <class U>
   polynomial& operator *=(const polynomial<U>& value)
   {
      // TODO: FIXME: use O(N log(N)) algorithm!!!
      BOOST_ASSERT(value.size());
      polynomial base(*this);
      *this *= value[0];
      for(size_type i = 1; i < value.size(); ++i)
      {
         polynomial t(base);
         t *= value[i];
         size_type s = size() - i;
         for(size_type j = 0; j < s; ++j)
         {
            m_data[i+j] += t[j];
         }
         for(size_type j = s; j < t.size(); ++j)
            m_data.push_back(t[j]);
      }
      return *this;
   }

   template <typename U>
   polynomial& operator /=(const polynomial<U>& value)
   {
       *this = quotient_remainder(*this, value).first;
       return *this;
   }
   
   template <typename U>
   polynomial& operator %=(const polynomial<U>& value)
   {
       *this = quotient_remainder(*this, value).second;
       return *this;
   }
private:
    template <class U, class R1, class R2>
    polynomial& linear_translation(const polynomial<U>& value, R1 sign, R2 op)
    {
        if (value != zero_element(std::multiplies< polynomial<U> >()))
        {
            size_type s1 = (std::min)(m_data.size(), value.size());
            for(size_type i = 0; i < s1; ++i)
                m_data[i] = op(m_data[i], value[i]);
            for(size_type i = s1; i < value.size(); ++i)
                m_data.push_back(sign(value[i]));
            while (!m_data.empty() && m_data.back() == U(0))
                m_data.pop_back();
        }
        return *this;
    }
    
    std::vector<T> m_data;
};


template <class T>
inline polynomial<T> operator + (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result += b;
   return result;
}

template <class T>
inline polynomial<T> operator - (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result -= b;
   return result;
}

template <class T>
inline polynomial<T> operator * (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result *= b;
   return result;
}

template <class T, class U>
inline polynomial<T> operator + (const polynomial<T>& a, const U& b)
{
   polynomial<T> result(a);
   result += b;
   return result;
}

template <class T, class U>
inline polynomial<T> operator - (const polynomial<T>& a, const U& b)
{
   polynomial<T> result(a);
   result -= b;
   return result;
}

template <class T, class U>
inline polynomial<T> operator * (const polynomial<T>& a, const U& b)
{
   polynomial<T> result(a);
   result *= b;
   return result;
}

template <class U, class T>
inline polynomial<T> operator + (const U& a, const polynomial<T>& b)
{
   polynomial<T> result(b);
   result += a;
   return result;
}

template <class U, class T>
inline polynomial<T> operator - (const U& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result -= b;
   return result;
}

template <class U, class T>
inline polynomial<T> operator * (const U& a, const polynomial<T>& b)
{
   polynomial<T> result(b);
   result *= a;
   return result;
}

template <class T>
bool operator == (const polynomial<T> &x, const polynomial<T> &y)
{
    return x.data() == y.data();
}

template <class charT, class traits, class T>
inline std::basic_ostream<charT, traits>& operator << (std::basic_ostream<charT, traits>& os, const polynomial<T>& poly)
{
   os << "{ ";
   for(unsigned i = 0; i < poly.size(); ++i)
   {
      if(i) os << ", ";
      os << poly[i];
   }
   os << " }";
   return os;
}

} // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_POLYNOMIAL_HPP



