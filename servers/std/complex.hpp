/*
 *  complex.hpp for Arduino
 * 
 *  This file is part of Lightweight C++ Standard Template Library (STL) for Arduino: https://github.com/BojanJurca/Lightweight-Standard-Template-Library-STL-for-Arduino
 *
 *  May 22, 2025, Bojan Jurca
 * 
 */


#ifndef __COMPLEX_HPP__
    #define __COMPLEX_HPP__


    #ifdef ARDUINO 


        // complex numbers for Arduino
            template<class T>
            class complex {
                private:
                    T _real_;
                    T _imag_;
                
                public:
                    // Constructor to initialize real and imag to 0
                    complex() : _real_ (0), _imag_ (0) {}
                    complex(T real, T imag) : _real_ (real), _imag_ (imag) {}
                
                    // real and imag parts
                    inline T real () const __attribute__((always_inline)) { return _real_; }
                    inline T imag () const __attribute__((always_inline)) { return _imag_; }
                
                    // + operator
                    template<typename t>
                    friend complex operator + (const complex<T>& obj1, const complex<t>& obj2) {
                        return {obj1.real () + obj2.real (), obj1.imag () + obj2.imag ()};
                    }
                
                    // += operator
                    complex& operator += (const complex<T>& other) {
                        _real_ += other.real ();
                        _imag_ += other.imag ();
                        return *this;
                    }
                
                    // - operator
                    template<typename t>
                    friend complex operator - (const complex<T>& obj1, const complex<t>& obj2) {
                        return {obj1.real () - obj2.real (), obj1.imag () - obj2.imag ()};
                    }
                
                    // -= operator
                    complex& operator -= (const complex<T>& other) {
                        _real_ -= other.real ();
                        _imag_ -= other.imag ();
                        return *this;
                    }
                
                    // * operator
                    template<typename t>
                    friend complex operator * (const complex<T>& obj1, const complex<t>& obj2) {
                        return {obj1.real () * obj2.real () - obj1.imag () * obj2.imag (), obj1.imag () * obj2.real () + obj1.real () * obj2.imag ()};
                    }
                
                    // *= operator
                    complex& operator *= (const complex<T>& other) {
                        T r = real () * other.real () - imag () * other.imag ();
                        T i = imag () * other.real () + real () * other.imag ();
                        _real_ = r;
                        _imag_ = i;
                        return *this;
                    }
                
                    // / operator
                    template<typename t>
                    friend complex operator / (const complex<T>& obj1, const complex<t>& obj2) {
                        T tmp = obj2.real () * obj2.real () + obj2.imag () * obj2.imag ();
                        return {(obj1.real () * obj2.real () + obj1.imag () * obj2.imag ()) / tmp, (obj1.imag () * obj2.real () - obj1.real () * obj2.imag ()) / tmp};
                    }
                
                    // /= operator
                    complex& operator /= (const complex<T>& other) {
                        T tmp = other.real () * other.real () + other.imag () * other.imag ();
                        T r = (real () * other.real () + imag () * other.imag ()) / tmp;
                        T i = (imag () * other.real () - real () * other.imag ()) / tmp;
                        _real_ = r;
                        _imag_ = i;
                        return *this;
                    }
                
                    // conjugate function
                    constexpr complex conj () const { return {real (), -imag ()}; }
                
                    // print complex number to ostream
                    friend ostream& operator << (ostream& os, const complex& c) {
                        os << c.real () << '+' << c.imag () << 'i';
                        return os;
                    }
            };

            // float exp (float x) { return expf (x); }

            // double exp (double x) { return expd (x); 

            complex<float> exp (complex<float> z) {
                float exp_real = expf (z.real ());
                return { exp_real * cos (z.imag ()), exp_real * sin (z.imag ()) };
            }

            complex<double> exp (complex<double> z) {
                double exp_real = exp (z.real ());
                return { exp_real * cos (z.imag ()), exp_real * sin (z.imag ()) };
            }

            // replace abs #definition with function template that would also handle complex numbers
            #ifdef abs
                #undef abs
            #endif

            template<typename T>
            T abs (T x) { return x >= 0 ? x : -x; }

            float abs (const complex<float>& z) { return sqrt (z.real () * z.real () + z.imag () * z.imag ()); }

            double abs (const complex<double>& z) { return sqrt (z.real () * z.real () + z.imag () * z.imag ()); }

    #endif

#endif
