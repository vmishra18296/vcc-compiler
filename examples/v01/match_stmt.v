// match_stmt.v — Pattern matching with match / _ default
// Demonstrates: match, integer patterns, wildcard _

module main

fun day_name(day: i64) {
    match day {
        1 => print("Monday")
        2 => print("Tuesday")
        3 => print("Wednesday")
        4 => print("Thursday")
        5 => print("Friday")
        6 => print("Saturday")
        7 => print("Sunday")
        _ => print("Invalid day")
    }
}

fun http_status(code: i64) {
    match code {
        200 => print("OK")
        201 => print("Created")
        400 => print("Bad Request")
        401 => print("Unauthorized")
        403 => print("Forbidden")
        404 => print("Not Found")
        500 => print("Internal Server Error")
        _   => print("Unknown status")
    }
}

fun traffic_light(state: i64) {
    // 0 = red, 1 = yellow, 2 = green
    match state {
        0 => print("STOP")
        1 => print("READY")
        2 => print("GO")
        _ => print("FAULT")
    }
}

fun boolean_name(flag: i64) {
    match flag {
        0 => print("FALSE")
        1 => print("TRUE")
        _ => print("UNKNOWN")
    }
}

fun main() {
    day_name(1)
    day_name(5)
    day_name(9)

    http_status(200)
    http_status(404)
    http_status(999)

    traffic_light(2)
    traffic_light(0)

    boolean_name(1)
    boolean_name(0)
    boolean_name(42)
}
