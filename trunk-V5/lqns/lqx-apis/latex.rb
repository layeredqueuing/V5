
module Latex
  
  def Latex.bf(string)
    return '\textbf{' + string + '}'
  end
  
  def Latex.em(string)
    return '\emph{' + string + '}'
  end
  
  def Latex.tt(string)
    return '{\tt ' + string + '}'
  end
  
  def Latex.begin_table(table_string)
    puts '\begin{tabular}{' + table_string + '}'
  end
  
  def Latex.end_table()
    puts '\end{tabular}'
  end
  
  def Latex.horizontal_line()
    puts '\hline'
  end
  
  def Latex.add_table_header(header)
    i = 0
    header.each do |item|
      print '\emph{' + item + '}'
      i = i + 1
      if i < header.size
        print ' & '
      end
    end
    puts '\\\\'
  end
  
  def Latex.multi_col_cell(span, fmt, text, fin=false)
    print '\multicolumn{' + span.to_s + '}{' + fmt + '}{' + text + '}';
    print '\\\\' unless fin == false
    puts ''
  end
  
  def Latex.add_row(row)
    i = 0
    row.each do |item|
      print item
      i = i + 1
      if i < row.size
        print ' & '
      end
    end
    puts '\\\\'
  end
  
  def Latex.end_line(n=1, s=0, fin=false)
    1.upto(s) do 
      print '\\\\'
    end
    1.upto(n) do
      print '\\\\\\ \\\\'
    end
    puts '' unless fin == false
  end
  
  def Latex.prep(str)
    str.gsub(/\_/, '\\_')
  end
  
  def Latex.subsection(str)
    puts '\subsection{' + str + '}'
  end
  
end
