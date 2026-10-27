
struct Config { base:u32,execution:u32,switches:u32,bins:u32 };
struct Params {v:array<f32>};
struct Ray {o:vec4<f32>,d:vec4<f32>};struct Spec {o:vec4<f32>,h:vec4<f32>};
struct Record {data:vec4<u32>,windows:array<vec2<f32>,8>};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> P:Params;
@group(0) @binding(2) var<storage,read> rays:array<Ray>;
@group(0) @binding(3) var<storage,read> specs:array<Spec>;
@group(0) @binding(4) var<storage,read_write> rootOutput:array<Record>;

fn switchSign_0(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((p).x) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_1(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_2(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_3(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_4(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_5(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_6(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_7(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * (((((p).z + (P.v[cb + 0] * sin((P.v[cb + 1] * (p).x + P.v[cb + 2])))) + (P.v[cb + 3] * cos((P.v[cb + 4] * (p).x + P.v[cb + 5])))) + (P.v[cb + 6] * sin((P.v[cb + 7] * (p).x + P.v[cb + 8]))))) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_8(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_9(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_10(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((p).x) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_11(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_12(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_13(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_14(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_15(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_16(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_17(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * (((((p).z + (P.v[cb + 0] * sin((P.v[cb + 1] * (p).x + P.v[cb + 2])))) + (P.v[cb + 3] * cos((P.v[cb + 4] * (p).x + P.v[cb + 5])))) + (P.v[cb + 6] * sin((P.v[cb + 7] * (p).x + P.v[cb + 8]))))) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_18(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_19(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_20(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((p).x) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_21(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_22(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_23(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_24(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_25(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_26(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_27(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * (((((p).z + (P.v[cb + 0] * sin((P.v[cb + 1] * (p).x + P.v[cb + 2])))) + (P.v[cb + 3] * cos((P.v[cb + 4] * (p).x + P.v[cb + 5])))) + (P.v[cb + 6] * sin((P.v[cb + 7] * (p).x + P.v[cb + 8]))))) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_28(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_29(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_30(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((p).x) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_31(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_32(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs((p).x)) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_33(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_34(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) * ((p).y + (P.v[cb + 1])))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_35(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_36(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((((p).y + (P.v[cb + 2]))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_37(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * (((((p).z + (P.v[cb + 0] * sin((P.v[cb + 1] * (p).x + P.v[cb + 2])))) + (P.v[cb + 3] * cos((P.v[cb + 4] * (p).x + P.v[cb + 5])))) + (P.v[cb + 6] * sin((P.v[cb + 7] * (p).x + P.v[cb + 8]))))) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_38(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}

fn switchSign_39(p: vec3<f32>, base: u32) -> f32 {
    let cb = base + 4u;
    var reach = -1.0;
    if (P.v[base + 0u] != 0.0) {
        let v0 = (P.v[base + 0u] * ((((P.v[cb + 0]) - select(0.0, ((abs(((((p).z + (P.v[cb + 2] * sin((P.v[cb + 3] * (p).x + P.v[cb + 4])))) + (P.v[cb + 5] * cos((P.v[cb + 6] * (p).x + P.v[cb + 7])))) + (P.v[cb + 8] * sin((P.v[cb + 9] * (p).x + P.v[cb + 10])))))) / ((P.v[cb + 1]))), abs((P.v[cb + 1])) >= 1e-06))) - P.v[base + 0u + 3u]) - P.v[base + 0u + 1u]) * P.v[base + 0u + 2u];
        if (v0 > 0.0) { return v0; }
        reach = max(reach, v0);
    }
    return reach;
}
fn switchSign(k:u32,p:vec3<f32>)->f32 {
 if(k==0u){return switchSign_0(p,0u);}
 if(k==1u){return switchSign_1(p,4u);}
 if(k==2u){return switchSign_2(p,10u);}
 if(k==3u){return switchSign_3(p,16u);}
 if(k==4u){return switchSign_4(p,22u);}
 if(k==5u){return switchSign_5(p,28u);}
 if(k==6u){return switchSign_6(p,35u);}
 if(k==7u){return switchSign_7(p,42u);}
 if(k==8u){return switchSign_8(p,55u);}
 if(k==9u){return switchSign_9(p,70u);}
 if(k==10u){return switchSign_10(p,85u);}
 if(k==11u){return switchSign_11(p,89u);}
 if(k==12u){return switchSign_12(p,95u);}
 if(k==13u){return switchSign_13(p,101u);}
 if(k==14u){return switchSign_14(p,107u);}
 if(k==15u){return switchSign_15(p,113u);}
 if(k==16u){return switchSign_16(p,120u);}
 if(k==17u){return switchSign_17(p,127u);}
 if(k==18u){return switchSign_18(p,140u);}
 if(k==19u){return switchSign_19(p,155u);}
 if(k==20u){return switchSign_20(p,170u);}
 if(k==21u){return switchSign_21(p,174u);}
 if(k==22u){return switchSign_22(p,180u);}
 if(k==23u){return switchSign_23(p,186u);}
 if(k==24u){return switchSign_24(p,192u);}
 if(k==25u){return switchSign_25(p,198u);}
 if(k==26u){return switchSign_26(p,205u);}
 if(k==27u){return switchSign_27(p,212u);}
 if(k==28u){return switchSign_28(p,225u);}
 if(k==29u){return switchSign_29(p,240u);}
 if(k==30u){return switchSign_30(p,255u);}
 if(k==31u){return switchSign_31(p,259u);}
 if(k==32u){return switchSign_32(p,265u);}
 if(k==33u){return switchSign_33(p,271u);}
 if(k==34u){return switchSign_34(p,277u);}
 if(k==35u){return switchSign_35(p,283u);}
 if(k==36u){return switchSign_36(p,290u);}
 if(k==37u){return switchSign_37(p,297u);}
 if(k==38u){return switchSign_38(p,310u);}
 if(k==39u){return switchSign_39(p,325u);}
 return bitcast<f32>(0x7fc00000u);
}

fn addRoot(v:ptr<function,Record>,a:f32,b:f32) {
 if((*v).data.x>=8u){(*v).data.y|=1u;return;}
 (*v).windows[(*v).data.x]=vec2<f32>(a,b);(*v).data.x+=1u;
}
@compute @workgroup_size(64)
fn rootMain(@builtin(global_invocation_id) gid:vec3<u32>) {
 if(gid.x>=cfg.execution||gid.y>=cfg.switches){return;}
 let index=gid.y*cfg.execution+gid.x;let ray=rays[gid.x%cfg.base];let spec=specs[gid.y];
 let o=ray.o.xyz-spec.o.xyz;let d=ray.d.xyz;var lo=ray.o.w;var hi=ray.d.w;
 var result:Record;result.data.w=index;
 for(var k=0u;k<3u;k+=1u) {
  if(abs(d[k])<1e-15) {if(abs(o[k])>spec.h[k]){result.data.y=4u;rootOutput[index]=result;return;}}
  else {let a=(-spec.h[k]-o[k])/d[k];let b=(spec.h[k]-o[k])/d[k];lo=max(lo,min(a,b));hi=min(hi,max(a,b));}
 }
 if(!(hi>lo)){result.data.y=4u;rootOutput[index]=result;return;}
 let pad=1e-4*(hi-lo);var previous=lo;var pv=switchSign(gid.y,o+lo*d);result.data.z=1u;
 for(var bin=0u;bin<cfg.bins;bin+=1u) {
  let end=lo+(hi-lo)*f32(bin+1u)/f32(cfg.bins);let ev=switchSign(gid.y,o+end*d);result.data.z+=1u;
  if(!(abs(ev)<=3.4e38 && abs(pv)<=3.4e38)){result.data.y|=2u;}
  else if(pv==0.0 && ev!=0.0) {addRoot(&result,max(lo,previous-pad),min(hi,previous+pad));}
  else if(ev==0.0 && pv!=0.0) {addRoot(&result,max(lo,end-pad),min(hi,end+pad));}
  else if((pv<0.0)!=(ev<0.0)) {
   var a=previous;var b=end;var av=pv;
   for(var it=0u;it<16u;it+=1u) {
    if(b-a<=pad*.125){break;}let mid=(a+b)*.5;let mv=switchSign(gid.y,o+mid*d);result.data.z+=1u;
    if(!(abs(mv)<=3.4e38)){result.data.y|=2u;break;}
    if((mv<0.0)==(av<0.0)){a=mid;av=mv;}else{b=mid;}
   }
   addRoot(&result,max(lo,a-pad),min(hi,b+pad));
  }
  previous=end;pv=ev;
 }
 rootOutput[index]=result;
}
@compute @workgroup_size(64)
fn emptyMain(@builtin(global_invocation_id) gid:vec3<u32>){
 if(gid.x==0u&&gid.y==0u){var v:Record;v.data.w=cfg.base+u32(P.v[0])+u32(rays[0].o.x)+u32(specs[0].h.x);rootOutput[0]=v;}
}
