require 'date'

class CustomFile
  class FileNotFoundError < RuntimeError; end

  FILE_HEADER_SIZE =  9 # magic_number (1) + next_block_link (8)
  EMPTY_LINKS_SYMBOL = '?'
  EMPTY_BYTES_SYMBOL = '_'
  FILENAME_SIZE = 11
  MAGIC_NUMBER = '0'

  attr_accessor :partition_name, # the partition name where the filter will be stored
    :name, :size, # file name and size
    :created_at, :updated_at, :touched_at, # timestamps
    :block_index, # the current block index
    :next_block_link, # the next block it uses if the current block is full
    :magic_number # a flag that indicates if the file is a directory (1) or not

  def initialize(partition_name)
    @partition_name = partition_name
    @magic_number = MAGIC_NUMBER
  end

  def create(name, block_index)
    set_timestamps
    @size = empty_size
    @next_link = empty_link
    @name = name.rjust(FILENAME_SIZE, ' ')
    @block_index = block_index.to_s.rjust(8, '0')
    File.open(partition_name, 'r+b') do |file|
      file.seek(block_index)
      (4000 - 8).times { file.write(EMPTY_BYTES_SYMBOL) }
      file.write(empty_link)
    end
  end

  def destroy
    block_index = @block_index
    loop do
      end_of_file = false
      File.open(partition_name, 'r+b') do |file|
        file.seek(block_index.to_i)
        (4000 - 8).times { file.write(EMPTY_BYTES_SYMBOL) }
        new_block_index = file.gets(8).dup
        file.rewind
        file.seek(block_index.to_i + 4000 - 8)
        file.write(EMPTY_BYTES_SYMBOL * 8)
        block_index = new_block_index
      end
      break if block_index == empty_link
    end

    self.freeze
    true
  end

  def write(string, file_manager = nil)
    @size = (@size.to_i + string.size).to_s.rjust(8, '0')

    if string.size < (4000 - 8)
      File.open(partition_name, 'r+b') do |file|
        file.seek(@block_index.to_i)
        file.write(string)
      end
    elsif file_manager.nil?
      raise RuntimeError.new
    else
      block_index = @block_index.to_i
      (string.size.to_f / (4000 - 8)).ceil.times do |i|
        if i > 0
          File.open(partition_name, 'r+b') do |file|
            file.seek(block_index + 4000 - 8)
            block_index = file_manager.new_block
            file.write(block_index.to_s.rjust(8, '0'))
          end
        end

        File.open(partition_name, 'r+b') do |file|
          file.seek(block_index)
          file.write(string.slice!(0, 4000 - 8).ljust(4000 - 8, EMPTY_BYTES_SYMBOL))
          file.write(empty_link)
        end
      end
    end
  end

  def read
    text = ""
    block_index = @block_index
    loop do
      File.open(partition_name, 'rb') do |file|
        file.seek(block_index.to_i)
        (4000 - 8).times do
          byte = file.getc
          return text if byte == EMPTY_BYTES_SYMBOL
          text << byte
        end
        block_index = file.gets(8)
        return text if block_index != empty_link
      end
    end

    text
  end

  def directory?
    false
  end

  def next_block_link
    next_block_link = 0
    File.open(partition_name, 'r+b') do |file|
      file.seek(block_index.to_i + 4000 - 8)
      next_block_link = file.gets(8)
    end
    next_block_link
  end

  def full_name(parent_directory)
    parent_directory.full_name + "/#{@name.strip}"
  end

  protected

    def empty_link
      EMPTY_LINKS_SYMBOL * 8
    end

    def empty_size
      FILE_HEADER_SIZE.to_s.rjust(8, '0')
    end

    def content_size
      @size.to_i
    end

    def set_timestamps
      date = date_time_now
      @created_at = date
      @updated_at = date
      @touched_at = date
    end

    def date_time_now
      DateTime.now.strftime("%Y%m%d%H%M%S")
    end
end
