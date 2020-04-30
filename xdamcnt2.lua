--
--       n Damen Problem loesen;
--       DAMCOUNT;
--       entworfen am 31.03.1985;
--       geschrieben am 02.04.1985;
--       revidiert am 18.04.1985 Do vorm.
--
--       in C++ umgewandelt am 17.04.1994
--       in Lua umgewandelt am 28.04.2020
--
--     2   4    6     8      10         12           14
--   1 0 0 2 10 4 40 92 352 724 2680 14200 73712 365596




-- Check if k-th queen is attacked by any other prior queen.
-- Return nonzero if configuration is OK, zero otherwise.

function configOkay (k, a)
	local z = a[k]
	local kmj
	local l

	for j=1, k-1 do
		l = z - a[j]
		kmj = k - j
		if (l == 0  or  l == kmj  or  -l == kmj) then
			return false
		end
	end
	return true
end



function solve (N, a)	-- return number of positions
	local cnt = 0
	local k = 1
	local N2 = N  --(N + 1) / 2;
	local flag
	a[1] = 1

	while true do
		flag = 0
		--print(string.format("\tN=%d, flag=%d, a[%d]=%d\n",N, flag, k, a[k]))
		if (configOkay(k,a)) then
			if (k < N) then
				k = k + 1;  a[k] = 1; flag = 1
			else
				cnt = cnt + 1; flag = 0
			end
		end
		if (flag == 0) then
			flag = 0
			repeat
				if (a[k] < N) then
					a[k] = a[k] + 1;  flag = 1; break;
				end
				k = k - 1
			until (k <= 1)
			if (flag == 0) then
				a[1] = a[1] + 1
				if (a[1] > N2) then return cnt; end
				k = 2;  a[2] = 1;
			end
		end
	end
end



-- Main program
local NMAX = 100
local a = {}

print([[
 n-queens problem.
   2   4    6     8      10         12           14
 1 0 0 2 10 4 40 92 352 724 2680 14200 73712 365596
]]
);

local start = 1
local endv = 12
print("#arg = " .. #arg)
if (#arg > 1) then endv = tonumber(arg[1]); end
if (endv <= 0  or  endv >= NMAX) then  endv = 10; end
if (#arg >= 2) then start = endv;  endv = tonumber(arg[2]); end
if (endv <= 0  or  endv >= NMAX) then endv = 10; end

for n=start, endv do
	print(string.format(" D(%2d) = %d", n, solve(n,a)))
end



