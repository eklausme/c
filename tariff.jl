# Tax Tariff
# See https://eklausmeier.wordpress.com/2020/06/16/gunnar-uldalls-tax-tariff/
# Elmar Klausmeier, 22-May-2020



# Tariff according Gunnar Uldall
function t_u(x)
	x *= 1.95583	# convert EUR to DEM
	if (x <= 12000) return 0 end
	if (x <= 20000)
		tax = 0.08 * (x - 12000)
	elseif (x <= 30000)
		tax = 0.18 * (x - 20000) + 640
	else
		tax = 0.28 * (x - 30000) + 2440
	end
	return tax / 1.95583	# convert DEM back to EUR
end



# Current tax tariff as of year 2020
function t_c(x)
	if (x <= 9408) return 0 end
	if (x <= 14532)
		y = (floor(x) - 9408) / 10000
		return (972.87*y + 1400) * y
	end
	if (x <= 57051)
		z = (floor(x) - 14532) / 10000
		return (212.02*z + 2397)*z + 972.79
	end
	if (x <= 270500) return 0.42*floor(x) - 8963.74 end
	return 0.45 * floor(x) - 17078.74
end

