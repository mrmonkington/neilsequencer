#encoding: latin-1

from eventbus import eventbus

hub = None

class Hub:
  """The hub loads and initializes all submodules. it also
  allows to register objects under a globally available id."""
  
  def __init__(self, modules):
    global hub
    hub = self
    self.registry = {}
    for modulename in modules:
      print "* importing module %s" % modulename
      module_ = __import__(modulename)
      names = modulename.split('.')
      for name in names[1:]:
        module_ = getattr(module_, name)
      for class_ in getattr(module_,'__HUBCLASSES__',[]):
        self.register(class_(self))
      
  def print_registry(self):
    for key,value in self.registry.items():
      print key + ' ::> ' + repr(value)
      
  def register(self, obj):
    hubinfo = getattr(obj, '__HUB__')
    id = hubinfo['id']
    assert id not in self.registry # make sure we dont register twice
    self.registry[id] = obj
    
  def resolve(self, id):
    return self.registry.get(id, None)
      
if __name__ == '__main__':
  import main
  main.main()
