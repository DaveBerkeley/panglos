#!/bin/env python

# By deepseek

from fractions import Fraction

def find_si5351_params(value, max_den=1048575):
    """Find a,b,c for Si5351A PLL/MultiSynth."""
    frac = Fraction(value).limit_denominator(max_den)
    a = frac.numerator // frac.denominator
    b = frac.numerator % frac.denominator
    c = frac.denominator
    return a, b, c

def generate_config(xtal_hz, outputs):
    """Generate Si5351A configuration."""
    print(f"// Si5351A configuration for {xtal_hz} MHz crystal")
    print()
    
    for name, freq_hz in outputs.items():
        print(f"// Output: {name} = {freq_hz} MHz")
        
        # Try to find a good integer divider
        # PLL must be 600-900 MHz
        min_pll = 600_000_000
        max_pll = 900_000_000
        
        best_divider = None
        best_pll = None
        best_error = float('inf')
        
        # Try integer dividers for the second stage
        valid = [ 4, 6, ] + list(range(8, 2048))
        for divider in valid:
            pll_freq = freq_hz * divider
            if min_pll <= pll_freq <= max_pll:
                # Check if this gives a clean PLL multiplier
                pll_mult = pll_freq / xtal_hz
                frac = Fraction(pll_mult).limit_denominator(0xfffff)
                error = abs(float(frac) - pll_mult)
                
                if error < best_error:
                    best_error = error
                    best_divider = divider
                    best_pll = pll_freq
                    best_pll_mult = pll_mult
        
        if best_divider is None:
            print(f"// WARNING: No integer divider found for {freq_hz} Hz")
            print()
            continue
        
        # Find PLL multiplier fraction
        a, b, c = find_si5351_params(best_pll_mult)
        actual_pll = (a + b/c) * xtal_hz
        actual_output = actual_pll / best_divider
        
        print(f"// Fout: {actual_output:.2f} Hz")
        print(f"// PLL: {actual_pll:.2f} Hz (divider {best_divider})")
        print(f"// PLL multiplier: {a} + {b}/{c} = {best_pll_mult:.10f}")
        print(f"// MultiSynth divider: {best_divider} (integer)")
        print(f"const uint32_t {name.upper()}_PLLx_A = {a};")
        print(f"const uint32_t {name.upper()}_PLLx_B = {b};")
        print(f"const uint32_t {name.upper()}_PLLx_C = {c};")
        print(f"const uint32_t {name.upper()}_MS_DIV = {best_divider};")
        print()

if __name__ == "__main__":
    import argparse

    p = argparse.ArgumentParser()
    p.add_argument('-x', '--xtal', default=25e6, type=float)
    p.add_argument('freq', nargs='*', type=float)
    args = p.parse_args()

    xtal_hz = args.xtal
    outputs = {}
    for i, f in enumerate(args.freq):
        outputs[f"clk{i}"] = f
    generate_config(xtal_hz, outputs)

#   FIN
