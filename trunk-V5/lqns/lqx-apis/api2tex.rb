
require 'rexml/document'
require 'latex'
include REXML

# ----------------------------------------------------- #

class Attribute
  
  attr_accessor :type, :name, :data
  
  def initialize()
    @type = "?"
    @name = ""
    @data = ""
  end
  
end

class APIMethod
  
  attr_accessor :returns, :name, :data, :args
  
  def initialize()
    @returns = "?"
    @args = []
    @name = ""
    @data = ""
  end
  
  def desc()
    s = "" + name + "("
    args.each do |a|
      s << a.desc
      if args[-1] != a
        s << ", "
      end
    end
    s << ")"
    return s
  end
  
end

class Argument
  
  attr_accessor :type, :name
  
  def initialize()
    @type = "?"
    @name = ""
  end
  
  def desc
    restr = {"1" => "opt", "+" => "..."}
    if restr.include?(@type)
      return restr[@type]
    end
    return "#{@type} #{@name}" 
  end
  
end

# ----------------------------------------------------- #

$info = {}
$constructors = []
$attributes = []

def make_attr(attribute)
  a = Attribute.new
  a.type = attribute.elements["type"].text
  a.name = attribute.elements["name"].text
  a.data = attribute.elements["data"].text
  return a
end

def make_arg(argument)
  a = Argument.new()
  a.type = argument.name
  a.name = argument.text
  return a
end

def make_method(method)
  if method.name != "method"
    puts "error: The tag named #{method.name} does not belong in the ctors section."
  else
    m = APIMethod.new
    m.returns = method.elements["ret"].text
    m.name = method.elements["name"].text
    m.data = method.elements["data"].text
    method.elements["args"].elements.each do |argument|
      m.args << make_arg(argument)
    end
    return m
  end
end

def decode_info(box)
  valid_info_tags = ["name", "binding", "provided-by"]
  box.elements.each do |tag|
    if valid_info_tags.include?(tag.name)
      $info[tag.name] = tag.text
    else
      puts "warning: The info key #{tag.name} is not valid. Ignored."
    end
  end
end

def decode_constructors(ctor_list)
  ctor_list.elements.each do |tag|
    method = make_method(tag)
    $constructors << method
  end
end

def decode_attributes(attr_list)
  attr_list.elements.each do |tag|
    a = make_attr(tag)
    $attributes << a
  end
end

# ----------------------------------------------------- #

if ARGV.size != 1
  puts "usage: api2tex.rb <input>"
  exit(1)
end

file = File.new(ARGV[0])
doc = Document.new(file)
root = doc.root

# Process all of the input information
decode_info(root.elements["info"])
decode_constructors(root.elements["constructors"])
decode_attributes(root.elements["attributes"])

# ----------------------------------------------------- #

Latex::subsection("#{$info["provided-by"]} Class: #{$info["name"]}")

# Print out the table of attributes
if $attributes.size > 0
  Latex::begin_table("|p{1.0in}|p{2.0in}||p{3in}|")
    Latex::horizontal_line
    Latex::multi_col_cell('3', '|l|', Latex::bf('Summary of Attributes'), true)
    Latex::horizontal_line
    $attributes.each do |a|
      Latex::add_row([a.type, Latex::tt(Latex::prep(a.name)), a.data])
    end
    Latex::horizontal_line
  Latex::end_table()
  Latex::end_line(1, 1, true)
end

if $constructors.size > 0
  Latex::begin_table("|p{1.0in}|p{2.0in}||p{3in}|")
    Latex::horizontal_line
    Latex::multi_col_cell('3', '|l|', Latex::bf('Summary of Constructors'), true)
    Latex::horizontal_line
    $constructors.each do |c|
      Latex::add_row([c.returns, Latex::tt(Latex::prep(c.desc)), c.data])
    end
    Latex::horizontal_line
  Latex::end_table()
end
