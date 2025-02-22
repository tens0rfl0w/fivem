return function()
    filter {}
    if os.istarget('windows') then
        add_dependencies { 'vendor:minhook' }
    end
end
