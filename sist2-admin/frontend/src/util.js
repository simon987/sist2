
export function formatBindAddress(address) {
    if (address.startsWith("0.0.0.0")) {
        return address.slice("0.0.0.0".length)
    }

    return address
}