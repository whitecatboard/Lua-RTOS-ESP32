http_response = ""

function print(...)
	local n = select("#",...)
	for i = 1,n do
		local v = tostring(select(i,...))
		http_response = http_response..v
		if i~=n then http_response = http_response..'\t' end
	end
	http_response = http_response..'\n'
end

function urldecode(s)
	s = s:gsub('+', ' ')
	     :gsub('%%(%x%x)',
	    function(h)
	 	return string.char(tonumber(h, 16))
	    end)
	return s
end                                        

function parseurl(s)
	--s = s:match('%s+(.+)')
	local ans = {}
	for k,v in s:gmatch('([^&=?]-)=([^&=?]+)' ) do
		ans[ k ] = urldecode(v)
	end
	return ans
end

get = nil
--get = parseurl(http_request)

