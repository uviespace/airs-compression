local pages = {
  ['programs/README.adoc'] = 'cli_guide.html',
  ['../programs/README.adoc'] = 'cli_guide.html',
  ['INSTALL.adoc'] = 'installation_guide.html',
  ['../INSTALL.adoc'] = 'installation_guide.html',
  ['README.adoc'] = 'index.html',
  ['../README.adoc'] = 'index.html',
}

function Link(el)
  local target = el.target:gsub('^#', '')
  if pages[target] then
    el.target = pages[target]
    el.classes = {}
  end
  return el
end
