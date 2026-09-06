@compute @workgroup_size(1)
fn main() {
    let a = select(1.0, 2.0, true);
    let b = select(1.0, 2.0, false);
}
