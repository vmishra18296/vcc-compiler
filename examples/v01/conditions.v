// conditions.v — Conditional statements using if / ef / el
// Demonstrates: if, ef (else-if), el (else), both braced and brace-less styles

module main

func classify_temperature(temp: i64) {
    // Braced style
    if temp > 50 {
        print("Scorching")
    }
    ef temp > 35 {
        print("Hot")
    }
    ef temp > 25 {
        print("Warm")
    }
    ef temp > 10 {
        print("Cool")
    }
    el {
        print("Cold")
    }
}

func check_score(score: i64) {
    // Brace-less style — single statement per branch
    if score >= 90
        print("Grade: A")
    ef score >= 80
        print("Grade: B")
    ef score >= 70
        print("Grade: C")
    ef score >= 60
        print("Grade: D")
    el
        print("Grade: F")
}

func is_even(n: i64) -> bool {
    if n % 2 == 0 {
        return true
    }
    el {
        return false
    }
}

func main() {
    classify_temperature(40)
    classify_temperature(20)
    classify_temperature(-5)

    check_score(95)
    check_score(72)
    check_score(55)
}
