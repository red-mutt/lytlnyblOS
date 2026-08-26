set architecture i8086

display/i (($cs << 4) + $pc)

define xi
    x/20i (($cs << 4) + $pc)
end

define sii
    si
    x/10i (($cs << 4) + $pc)
end
