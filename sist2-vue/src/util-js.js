export function mergeTooltips(slider, threshold, separator, fixTooltips) {

    const isMobile = window.innerWidth <= 650;
    if (isMobile) {
        threshold = 25;
    }

    const textIsRtl = getComputedStyle(slider).direction === 'rtl';
    const isRtl = slider.noUiSlider.options.direction === 'rtl';
    const isVertical = slider.noUiSlider.options.orientation === 'vertical';
    const tooltips = slider.noUiSlider.getTooltips();
    const origins = slider.noUiSlider.getOrigins();

    // Move tooltips into the origin element. The default stylesheet handles this.
    tooltips.forEach(function (tooltip, index) {
        if (tooltip) {
            origins[index].appendChild(tooltip);
        }
    });

    slider.noUiSlider.on('update', function (values, handle, unencoded, tap, positions) {

        const pools = [[]];
        const poolPositions = [[]];
        const poolValues = [[]];
        let atPool = 0;

        // Assign the first tooltip to the first pool, if the tooltip is configured
        if (tooltips[0]) {
            pools[0][0] = 0;
            poolPositions[0][0] = positions[0];
            poolValues[0][0] = values[0];
        }

        for (let i = 1; i < positions.length; i++) {
            if (!tooltips[i] || (positions[i] - positions[i - 1]) > threshold) {
                atPool++;
                pools[atPool] = [];
                poolValues[atPool] = [];
                poolPositions[atPool] = [];
            }

            if (tooltips[i]) {
                pools[atPool].push(i);
                poolValues[atPool].push(values[i]);
                poolPositions[atPool].push(positions[i]);
            }
        }

        pools.forEach(function (pool, poolIndex) {
            const handlesInPool = pool.length;

            for (let j = 0; j < handlesInPool; j++) {
                const handleNumber = pool[j];

                if (j === handlesInPool - 1) {
                    let offset = 0;

                    poolPositions[poolIndex].forEach(function (value) {
                        offset += 1000 - 10 * value;
                    });

                    const direction = isVertical ? 'bottom' : 'right';
                    const last = isRtl ? 0 : handlesInPool - 1;
                    const lastOffset = 1000 - 10 * poolPositions[poolIndex][last];
                    offset = (textIsRtl && !isVertical ? 100 : 0) + (offset / handlesInPool) - lastOffset;

                    // Center this tooltip over the affected handles
                    tooltips[handleNumber].innerHTML = poolValues[poolIndex].join(separator);
                    tooltips[handleNumber].style.display = 'block';

                    tooltips[handleNumber].style[direction] = offset + '%';
                } else {
                    // Hide this tooltip
                    tooltips[handleNumber].style.display = 'none';
                }
            }
        });

        if (fixTooltips) {
            const isMobile = window.innerWidth <= 650;
            const len = isMobile ? 20 : 5;

            if (positions[0] < len) {
                tooltips[0].style.right = `${(1 - ((positions[0]) / len)) * -35}px`
            } else {
                tooltips[0].style.right = "0"
            }

            if (positions[1] > (100 - len)) {
                tooltips[1].style.right = `${((positions[1] - (100 - len)) / len) * 35}px`
            } else {
                tooltips[1].style.right = "0"
            }
        }
    });
}


export function burrow(table, addSelfDir, rootName) {
    const root = {};
    table.forEach(row => {
        let layer = root;

        row.taxonomy.forEach(key => {
            layer[key] = key in layer ? layer[key] : {};
            layer = layer[key];
        });
        if (Object.keys(layer).length === 0) {
            layer["$size$"] = row.size;
        } else if (addSelfDir) {
            layer["."] = {
                "$size$": row.size,
            };
        }
    });

    const descend = function (obj, depth) {
        return Object.keys(obj).filter(k => k !== "$size$").map(k => {
            const child = {
                name: k,
                depth: depth,
                value: 0,
                children: descend(obj[k], depth + 1)
            };
            if ("$size$" in obj[k]) {
                child.value = obj[k]["$size$"];
            }
            return child;
        });
    };

    return {
        name: rootName,
        children: descend(root, 1),
        value: 0,
        depth: 0,
    }
}
